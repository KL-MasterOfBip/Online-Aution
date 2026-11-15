"""Integration checks for interactive authentication and terminal password echo."""
import os
from pathlib import Path
import pty
import select
import signal
import socket
import subprocess
import tempfile
import termios
import time
import unittest
import uuid

ROOT = Path(__file__).resolve().parents[1]


class AuthTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix='auction-auth-')
        cls.work = Path(cls.temp.name)
        cls.addClassCleanup(cls.temp.cleanup)
        with socket.socket() as probe:
            probe.bind(('127.0.0.1', 0))
            cls.port = probe.getsockname()[1]
        for name in ('server', 'client'):
            subprocess.run(['gcc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic',
                            f'-DPORT={cls.port}', str(ROOT / 'src' / f'{name}.c'),
                            '-o', str(cls.work / name)], check=True)
        cls.server = subprocess.Popen([str(cls.work / 'server')], cwd=cls.work,
                                      stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        cls.addClassCleanup(cls.stop_server)
        for _ in range(100):
            try:
                with socket.create_connection(('127.0.0.1', cls.port), timeout=.2):
                    return
            except OSError:
                if cls.server.poll() is not None:
                    raise RuntimeError(cls.server.stderr.read().decode())
                time.sleep(.02)
        raise RuntimeError('Server did not start')

    @classmethod
    def stop_server(cls):
        cls.server.terminate()
        cls.server.wait(timeout=3)
        cls.server.stderr.close()

    def connect(self):
        s = socket.create_connection(('127.0.0.1', self.port), timeout=2)
        f = s.makefile('rb')
        self.addCleanup(s.close)
        self.addCleanup(f.close)
        self.assertEqual(f.readline(), b'OK Connected to auction server\n')
        return s, f

    def send(self, peer, text):
        s, f = peer
        s.sendall((text + '\n').encode())
        return f.readline().decode().rstrip('\n')

    def name(self):
        return 'u' + uuid.uuid4().hex[:15]

    def register(self, peer, name, password='secret-pass'):
        self.assertEqual(self.send(peer, 'REGISTER'), 'New username:')
        self.assertEqual(self.send(peer, name), 'New password:')
        self.assertEqual(self.send(peer, password), 'OK Account registered')

    def test_register_login_logout_and_guest_only(self):
        peer = self.connect()
        name = self.name()
        self.register(peer, name)
        self.assertEqual(self.send(peer, 'LOGIN'), 'Username:')
        self.assertEqual(self.send(peer, name), 'Password:')
        self.assertEqual(self.send(peer, 'secret-pass'), 'OK Login successful')
        self.assertEqual(self.send(peer, 'REGISTER'), 'ERROR Cannot register while logged in')
        self.assertEqual(self.send(peer, 'REGISTER other pass'), 'ERROR Cannot register while logged in')
        self.assertEqual(self.send(peer, 'LOGIN'), 'ERROR Already logged in')
        self.assertEqual(self.send(peer, 'LOGOUT'), 'OK Logout successful')
        self.register(peer, self.name())

    def test_duplicate_username_retries_and_concurrent_registration(self):
        a, b = self.connect(), self.connect()
        name = self.name()
        self.assertEqual(self.send(a, 'REGISTER'), 'New username:')
        self.assertEqual(self.send(a, name), 'New password:')
        self.register(b, name)
        self.assertEqual(self.send(a, 'pass'), 'ERROR Username already exists')
        self.assertEqual(a[1].readline(), b'New username:\n')
        self.assertEqual(self.send(a, name), 'ERROR Username already exists')
        self.assertEqual(a[1].readline(), b'New username:\n')
        self.assertEqual(self.send(a, self.name()), 'New password:')
        self.assertEqual(self.send(a, 'pass'), 'OK Account registered')

    def test_invalid_input_retries_and_login_failure(self):
        a = self.connect()
        self.assertEqual(self.send(a, 'REGISTER'), 'New username:')
        for text in ('', 'two names', 'a' * 32):
            self.assertTrue(self.send(a, text).startswith('ERROR Username'))
            self.assertEqual(a[1].readline(), b'New username:\n')
        name = self.name()
        self.assertEqual(self.send(a, name), 'New password:')
        for text in ('', 'two words', 'a' * 64):
            self.assertTrue(self.send(a, text).startswith('ERROR Password'))
            self.assertEqual(a[1].readline(), b'New password:\n')
        self.assertEqual(self.send(a, 'pass'), 'OK Account registered')
        self.assertEqual(self.send(a, 'LOGIN'), 'Username:')
        self.assertEqual(self.send(a, name), 'Password:')
        self.assertEqual(self.send(a, 'wrong'), 'ERROR Invalid username or password')
        self.assertEqual(self.send(a, 'PING'), 'OK PONG')

    def test_framing_and_auth_input_not_commands_or_log_entries(self):
        a = self.connect()
        name = self.name()
        password = 'private-' + self.name()
        a[0].sendall(('REG').encode())
        a[0].sendall(('ISTER\n' + name + '\n' + password + '\nPING\n').encode())
        self.assertEqual([a[1].readline() for _ in range(4)],
                         [b'New username:\n', b'New password:\n', b'OK Account registered\n', b'OK PONG\n'])
        log = (self.work / 'auction_server.log').read_text()
        self.assertNotIn(password, log)
        self.assertNotIn('command=' + name, log)
        # QUIT at a password prompt is data, not a request to close the socket.
        name2 = self.name()
        self.assertEqual(self.send(a, 'REGISTER'), 'New username:')
        self.assertEqual(self.send(a, name2), 'New password:')
        self.assertEqual(self.send(a, 'QUIT'), 'OK Account registered')
        self.assertEqual(self.send(a, 'PING'), 'OK PONG')

    def test_sessions_independent_and_old_commands_work(self):
        a, b = self.connect(), self.connect()
        name = self.name()
        self.assertEqual(self.send(a, 'REGISTER'), 'New username:')
        self.assertEqual(self.send(b, 'PING'), 'OK PONG')
        self.assertEqual(self.send(b, f'REGISTER {name} pass'), 'OK Account registered')
        self.assertEqual(self.send(b, f'LOGIN {name} pass'), 'OK Login successful')
        self.assertEqual(self.send(b, 'CREATE_ROOM ' + self.name()).split()[0:2], ['OK', 'ROOM_CREATED'])
        self.assertEqual(self.send(b, 'LOGOUT'), 'OK Logout successful')
        # Disconnecting during a prompt does not keep state for a new connection.
        a[1].close()
        a[0].close()
        c = self.connect()
        self.assertEqual(self.send(c, 'PING'), 'OK PONG')
        self.assertEqual(self.send(c, '  QUIT  '), 'OK Goodbye')
        self.assertEqual(c[1].readline(), b'')
        self.assertEqual(self.send(b, 'QUIT extra'), 'ERROR Invalid command or arguments')
        self.assertEqual(self.send(b, 'PING'), 'OK PONG')

    def test_room_prompts_validation_join_by_id_and_name_response(self):
        peer = self.connect()
        self.assertEqual(self.send(peer, 'CREATE_ROOM'), 'ERROR Not logged in')
        self.assertEqual(self.send(peer, 'JOIN_ROOM'), 'ERROR Not logged in')
        user = self.name()
        self.register(peer, user)
        self.assertEqual(self.send(peer, f'LOGIN {user} secret-pass'), 'OK Login successful')
        room = self.name()
        self.assertEqual(self.send(peer, 'CREATE_ROOM'), 'Room name:')
        for invalid in ('', 'a' * 50, 'two words'):
            self.assertTrue(self.send(peer, invalid).startswith('ERROR Room name'))
            self.assertEqual(peer[1].readline(), b'Room name:\n')
        created = self.send(peer, room)
        self.assertTrue(created.startswith('OK ROOM_CREATED'))
        room_id = created.split()[-1]
        self.assertEqual(self.send(peer, 'CREATE_ROOM'), 'Room name:')
        self.assertEqual(self.send(peer, room), 'ERROR Room already exists')
        self.assertEqual(peer[1].readline(), b'Room name:\n')
        self.assertTrue(self.send(peer, self.name()).startswith('OK ROOM_CREATED'))
        self.assertEqual(self.send(peer, 'JOIN_ROOM'), 'Room ID:')
        for invalid in ('', 'abc', '0', '-1', '2147483648', '1 extra'):
            self.assertEqual(self.send(peer, invalid), 'ERROR Invalid room ID')
            self.assertEqual(peer[1].readline(), b'Room ID:\n')
        self.assertEqual(self.send(peer, '2147483647'), 'ERROR Room not found')
        self.assertEqual(peer[1].readline(), b'Room ID:\n')
        self.assertEqual(self.send(peer, room_id), 'OK JOINED_ROOM ' + room)
        self.assertEqual(self.send(peer, 'JOIN_ROOM'), 'ERROR Already in a room')
        self.assertEqual(self.send(peer, 'LEAVE_ROOM'), 'OK LEFT_ROOM')
        self.assertEqual(self.send(peer, 'JOIN_ROOM ' + room_id), 'OK JOINED_ROOM ' + room)

    def test_room_terminal_prompts_echo_and_independent_clients(self):
        process, master, slave = self.start_terminal()
        self.read_until(master, b'OK Connected')
        name = self.name()
        os.write(master, (f'REGISTER {name} pass\nLOGIN {name} pass\n').encode())
        self.read_until(master, b'OK Login successful')
        os.write(master, b'CREATE_ROOM\n')
        self.read_until(master, b'Room name: ')
        self.assertTrue(termios.tcgetattr(slave)[3] & termios.ECHO)
        other = self.connect()
        self.assertEqual(self.send(other, 'PING'), 'OK PONG')
        room = self.name()
        os.write(master, (room + '\n').encode())
        output = self.read_until(master, b'OK ROOM_CREATED')
        # Finish reading the response if TCP/PTY splits it before the ID.
        while b'\n' not in output[output.index(b'OK ROOM_CREATED'):]:
            output += os.read(master, 4096)
        room_id = output.split(b'OK ROOM_CREATED', 1)[1].splitlines()[0].split()[-1]
        os.write(master, b'JOIN_ROOM\n')
        self.read_until(master, b'Room ID: ')
        self.assertTrue(termios.tcgetattr(slave)[3] & termios.ECHO)
        os.write(master, room_id + b'\n')
        self.read_until(master, ('OK JOINED_ROOM ' + room).encode())
        os.write(master, b'QUIT\n')
        self.assertEqual(process.wait(timeout=3), 0)

    def start_terminal(self):
        master, slave = pty.openpty()
        process = subprocess.Popen([str(self.work / 'client'), '127.0.0.1', str(self.port)],
                                   stdin=slave, stdout=slave, stderr=slave)
        def cleanup():
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=3)
            os.close(master)
            os.close(slave)
        self.addCleanup(cleanup)
        return process, master, slave

    def read_until(self, fd, needle):
        output = b''
        deadline = time.monotonic() + 3
        while needle not in output and time.monotonic() < deadline:
            if select.select([fd], [], [], .1)[0]:
                try:
                    chunk = os.read(fd, 4096)
                except OSError:
                    break
                if not chunk:
                    break
                output += chunk
        self.assertIn(needle, output)
        return output

    def password_prompt(self, master):
        self.read_until(master, b'OK Connected')
        os.write(master, b'REGISTER\n')
        self.read_until(master, b'New username: ')
        os.write(master, (self.name() + '\n').encode())
        self.read_until(master, b'New password: ')

    def test_terminal_password_hidden_for_register_and_login(self):
        process, master, slave = self.start_terminal()
        self.read_until(master, b'OK Connected')
        name = self.name()
        for command, username_prompt, password_prompt, success in (
            ('REGISTER', b'New username: ', b'New password: ', b'OK Account registered'),
            ('LOGIN', b'Username: ', b'Password: ', b'OK Login successful'),
        ):
            os.write(master, (command + '\n').encode())
            self.read_until(master, username_prompt)
            os.write(master, (name + '\n').encode())
            out = self.read_until(master, password_prompt)
            self.assertIn(name.encode(), out)
            self.assertFalse(termios.tcgetattr(slave)[3] & termios.ECHO)
            os.write(master, b'not-visible-pass\n')
            out = self.read_until(master, success)
            self.assertNotIn(b'not-visible-pass', out)
            self.assertTrue(termios.tcgetattr(slave)[3] & termios.ECHO)
        os.write(master, b'QUIT\n')
        self.read_until(master, b'OK Goodbye')
        self.assertEqual(process.wait(timeout=3), 0)

    def test_terminal_restores_echo_on_interrupt(self):
        process, master, slave = self.start_terminal()
        self.password_prompt(master)
        self.assertFalse(termios.tcgetattr(slave)[3] & termios.ECHO)
        process.send_signal(signal.SIGINT)
        process.wait(timeout=3)
        self.assertTrue(termios.tcgetattr(slave)[3] & termios.ECHO)

    def test_terminal_restores_echo_on_eof(self):
        process, master, slave = self.start_terminal()
        self.password_prompt(master)
        os.write(master, b'\x04')
        process.wait(timeout=3)
        self.assertTrue(termios.tcgetattr(slave)[3] & termios.ECHO)


if __name__ == '__main__':
    unittest.main(verbosity=2)
