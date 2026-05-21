import subprocess
import socket
import json
import time
import sys
import shutil
import uuid
from pathlib import Path

def find_project_root(start_path):
    current = Path(start_path).resolve()
    for parent in [current] + list(current.parents):
        if (parent / "CMakeLists.txt").exists() and (parent / "scripts").exists():
            return parent
    raise RuntimeError("Не удалось найти корень проекта (CMakeLists.txt и scripts/)")

SCRIPT_DIR = Path(__file__).parent.resolve()
PROJECT_ROOT = find_project_root(SCRIPT_DIR)

def print_header(title):
    print("\n" + "=" * 60)
    print(f"  {title}")
    print("=" * 60)

def print_ok(msg):
    print(f"✓ {msg}")

def print_fail(msg):
    print(f"✗ {msg}")

def print_info(msg):
    print(f"ℹ {msg}")

def run_cmd(cmd, cwd=None, shell=True, capture=True):
    if isinstance(cmd, list) and not shell:
        proc = subprocess.run(cmd, cwd=cwd, capture_output=capture, text=True)
    else:
        proc = subprocess.run(cmd, cwd=cwd, shell=shell, capture_output=capture, text=True)
    return proc.returncode, proc.stdout, proc.stderr

def wait_for_port(host, port, timeout=5):
    start = time.time()
    while time.time() - start < timeout:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            if sock.connect_ex((host, port)) == 0:
                return True
        time.sleep(0.5)
    return False

def send_sql_query(host, port, query):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.settimeout(5)
            sock.connect((host, port))
            query_bytes = query.encode('utf-8')
            length = len(query_bytes).to_bytes(4, byteorder='big')
            sock.send(length + query_bytes)
            status = sock.recv(4)
            if len(status) < 4:
                return {"type": "error", "message": "Incomplete status"}
            data_len_bytes = sock.recv(4)
            if len(data_len_bytes) < 4:
                return {"type": "error", "message": "Incomplete length"}
            data_len = int.from_bytes(data_len_bytes, byteorder='big')
            data = b''
            while len(data) < data_len:
                chunk = sock.recv(data_len - len(data))
                if not chunk:
                    break
                data += chunk
            return json.loads(data.decode('utf-8'))
    except Exception as e:
        return {"type": "error", "message": str(e)}

def kill_process_on_port(port):
    """Убивает процесс, занимающий указанный порт (Windows/Linux)"""
    import platform
    if platform.system() == "Windows":
        result = subprocess.run(f"netstat -aon | findstr :{port}", shell=True, capture_output=True, text=True)
        lines = result.stdout.strip().split('\n')
        pids = set()
        for line in lines:
            if "LISTENING" in line:
                parts = line.split()
                if len(parts) >= 5:
                    pid = parts[-1]
                    pids.add(pid)
        for pid in pids:
            subprocess.run(f"taskkill /F /PID {pid}", shell=True, capture_output=True)
            print_info(f"Убит процесс с PID {pid}, занимавший порт {port}")
    else:
        subprocess.run(f"fuser -k {port}/tcp", shell=True, capture_output=True)

class SystemTests:
    def __init__(self):
        self.is_windows = sys.platform == "win32"
        self.project_root = PROJECT_ROOT
        self.build_script = self.project_root / "scripts" / ("build_full.bat" if self.is_windows else "build_full.sh")
        self.server_exe = None
        self.docker_image = "databaseoficial-customdb-server"
        self.host = "localhost"
        self.port = 5432
        self.docker_available = False
        self.data_dir = self.project_root / "data"
        print_info(f"Корень проекта: {self.project_root}")

    def cleanup_environment(self):
        """Останавливает все контейнеры, убивает процессы на порту, удаляет папку data"""
        print_header("Предварительная очистка окружения")
        # Останавливаем и удаляем контейнер customdb-server (если есть)
        run_cmd("docker rm -f customdb-server", shell=True)
        run_cmd("docker rm -f customdb_test_container", shell=True)
        # Убиваем процессы на порту 5432
        kill_process_on_port(self.port)
        # Удаляем папку data
        if self.data_dir.exists():
            shutil.rmtree(self.data_dir, ignore_errors=True)
            print_ok(f"Удалена папка {self.data_dir}")
        time.sleep(1)
        print_ok("Окружение очищено")

    def check_prerequisites(self):
        print_header("Проверка наличия необходимых инструментов")
        ret, out, err = run_cmd("cmake --version")
        if ret == 0:
            print_ok(f"CMake найден: {out.strip().splitlines()[0]}")
        else:
            print_fail("CMake не найден")
            return False
        ret, out, err = run_cmd("dotnet --version")
        if ret == 0:
            print_ok(f".NET SDK: {out.strip()}")
        else:
            print_fail(".NET SDK не найден")
            return False
        ret, out, err = run_cmd("docker --version")
        if ret == 0:
            print_ok(f"Docker найден: {out.strip()}")
            self.docker_available = True
        else:
            print_info("Docker не найден (тесты Docker будут пропущены)")
        return True

    def build_project(self):
        print_header("Сборка проекта (скрипт build_full)")
        if not self.build_script.exists():
            print_fail(f"Скрипт сборки не найден: {self.build_script}")
            return False
        ret, out, err = run_cmd(str(self.build_script), cwd=str(self.project_root))
        if ret != 0:
            print_fail(f"Скрипт сборки завершился с кодом {ret}")
            return False
        print_ok("Скрипт сборки выполнен")
        if self.is_windows:
            self.server_exe = self.project_root / "build" / "Release" / "customdb_server.exe"
        else:
            self.server_exe = self.project_root / "build" / "customdb_server"
        if not self.server_exe.exists():
            print_fail(f"Сервер не найден: {self.server_exe}")
            return False
        print_ok(f"Сервер найден: {self.server_exe}")
        if self.is_windows:
            gui_exe = self.project_root / "publish" / "win-x64" / "CustomDB.UI.exe"
            if gui_exe.exists():
                print_ok(f"GUI найден: {gui_exe}")
        return True

    def build_docker_image(self):
        print_header("Сборка Docker-образа")
        if not self.docker_available:
            print_info("Docker не доступен, пропускаем")
            return True
        ret, out, err = run_cmd(f"docker build -t {self.docker_image} .", cwd=str(self.project_root))
        if ret != 0:
            print_fail(f"Ошибка сборки Docker: {err[:300]}")
            return False
        print_ok(f"Docker-образ {self.docker_image} собран")
        return True

    def test_native_server(self):
        print_header("Тестирование локального сервера")
        # Гарантируем чистую папку данных и свободный порт
        if self.data_dir.exists():
            shutil.rmtree(self.data_dir, ignore_errors=True)
        kill_process_on_port(self.port)
        time.sleep(1)
        proc = None
        try:
            proc = subprocess.Popen([str(self.server_exe)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=str(self.server_exe.parent))
            if not wait_for_port(self.host, self.port, timeout=10):
                print_fail("Сервер не запустился за отведённое время")
                return False
            print_ok("Сервер запущен")
            queries = [
                ("CREATE DATABASE testdb;", "ddl", True),
                ("USE testdb;", "ddl", True),
                ("CREATE TABLE users (id INT, name TEXT);", "ddl", True),
                ("INSERT INTO users VALUES (1, 'Alice');", "dml", None),
            ]
            for query, expected_type, _ in queries:
                resp = send_sql_query(self.host, self.port, query)
                if resp.get("type") == "error":
                    print_fail(f"Ошибка запроса: {query} -> {resp.get('message')}")
                    return False
                if expected_type and resp.get("type") != expected_type:
                    print_fail(f"Ожидался тип {expected_type}, получен {resp.get('type')}")
                    return False
                print_ok(f"Запрос выполнен: {query[:40]}...")
            select_resp = send_sql_query(self.host, self.port, "SELECT * FROM users;")
            if select_resp.get("type") != "select":
                print_fail("SELECT не выполнен")
                return False
            rows = select_resp.get("rows", [])
            expected_value = "Alice"
            actual_value = rows[0][1] if rows else ""
            if actual_value not in (expected_value, f"'{expected_value}'"):
                print_fail(f"Неверный результат SELECT: {rows}")
                return False
            print_ok("SELECT вернул корректные данные")
            return True
        finally:
            if proc:
                proc.terminate()
                proc.wait(timeout=5)
                if proc.poll() is None:
                    proc.kill()
            kill_process_on_port(self.port)

    def test_docker_server(self):
        print_header("Тестирование сервера в Docker")
        if not self.docker_available:
            print_info("Docker не доступен, пропускаем")
            return True
        container = "customdb_test_container"
        # Очистка перед запуском
        run_cmd(f"docker rm -f {container}", shell=True)
        kill_process_on_port(self.port)
        time.sleep(1)
        ret, out, err = run_cmd(f"docker run -d --name {container} -p {self.port}:{self.port} {self.docker_image}")
        if ret != 0:
            print_fail(f"Не удалось запустить контейнер: {err}")
            return False
        if not wait_for_port(self.host, self.port, timeout=10):
            print_fail("Контейнер запущен, но порт не отвечает")
            run_cmd(f"docker rm -f {container}")
            return False
        print_ok("Docker-контейнер работает")
        resp = send_sql_query(self.host, self.port, "CREATE DATABASE docker_test;")
        if resp.get("type") != "ddl" or not resp.get("success"):
            print_fail(f"Ошибка запроса в контейнере: {resp}")
            run_cmd(f"docker rm -f {container}")
            return False
        print_ok("SQL-запрос выполнен внутри контейнера")
        run_cmd(f"docker rm -f {container}")
        return True

    def test_persistence(self):
        print_header("Тестирование сохранения данных (персистентность)")
        db_name = f"persist_{uuid.uuid4().hex[:8]}"
        # Очищаем папку данных и порт
        if self.data_dir.exists():
            shutil.rmtree(self.data_dir, ignore_errors=True)
        kill_process_on_port(self.port)
        time.sleep(1)
        proc = None
        try:
            proc = subprocess.Popen([str(self.server_exe)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=str(self.server_exe.parent))
            if not wait_for_port(self.host, self.port, timeout=10):
                print_fail("Сервер не запустился")
                return False
            print_ok("Сервер запущен")
            resp = send_sql_query(self.host, self.port, f"CREATE DATABASE {db_name};")
            if resp.get("type") != "ddl" or not resp.get("success"):
                print_fail(f"Не удалось создать БД {db_name}: {resp}")
                return False
            send_sql_query(self.host, self.port, f"USE {db_name};")
            send_sql_query(self.host, self.port, "CREATE TABLE test (id INT, val TEXT);")
            send_sql_query(self.host, self.port, "INSERT INTO test VALUES (1, 'hello');")
            proc.terminate()
            proc.wait(timeout=5)
            if proc.poll() is None:
                proc.kill()
            proc = None
            # Запускаем заново
            proc = subprocess.Popen([str(self.server_exe)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=str(self.server_exe.parent))
            if not wait_for_port(self.host, self.port, timeout=10):
                print_fail("Сервер не перезапустился")
                return False
            resp = send_sql_query(self.host, self.port, f"USE {db_name}; SELECT * FROM test;")
            if resp.get("type") != "select":
                print_fail(f"Данные не сохранены: {resp}")
                return False
            rows = resp.get("rows", [])
            if len(rows) == 1 and rows[0][1] == "hello":
                print_ok("Данные сохранены и восстановлены")
                return True
            else:
                print_fail(f"Неверные данные после перезапуска: {rows}")
                return False
        finally:
            if proc:
                proc.terminate()
                proc.wait(timeout=5)
                if proc.poll() is None:
                    proc.kill()
            # Очищаем папку данных после теста, чтобы не мешать другим
            if self.data_dir.exists():
                shutil.rmtree(self.data_dir, ignore_errors=True)
            kill_process_on_port(self.port)

    def test_gui_basic(self):
        print_header("Базовая проверка GUI (только Windows)")
        if not self.is_windows:
            print_info("GUI тест пропущен (не Windows)")
            return True
        gui_exe = self.project_root / "publish" / "win-x64" / "CustomDB.UI.exe"
        if not gui_exe.exists():
            print_info("GUI не собран, пропускаем")
            return True
        try:
            proc = subprocess.Popen([str(gui_exe)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            time.sleep(2)
            proc.terminate()
            proc.wait(timeout=3)
            print_ok("GUI запущен и завершил работу без краха")
            return True
        except Exception as e:
            print_fail(f"Ошибка при запуске GUI: {e}")
            return False

    def cleanup(self):
        print_header("Очистка временных данных")
        if self.data_dir.exists():
            shutil.rmtree(self.data_dir, ignore_errors=True)
            print_ok(f"Удалена папка {self.data_dir}")
        if self.docker_available:
            run_cmd("docker rm -f customdb_test_container", shell=True)
            run_cmd("docker rm -f customdb-server", shell=True)
        kill_process_on_port(self.port)
        print_ok("Очистка завершена")

    def run_all(self):
        print_header("ЗАПУСК ФУНКЦИОНАЛЬНЫХ ТЕСТОВ")
        # Предварительная очистка окружения
        self.cleanup_environment()

        if not self.check_prerequisites():
            return 1
        if not self.build_project():
            return 1
        if not self.build_docker_image():
            pass

        tests = [
            ("Локальный сервер", self.test_native_server),
            ("Docker-сервер", self.test_docker_server),
            ("Персистентность", self.test_persistence),
            ("GUI", self.test_gui_basic),
        ]
        failures = 0
        for name, test_func in tests:
            print_header(f"ТЕСТ: {name}")
            try:
                if test_func():
                    print_ok(f"Тест '{name}' пройден")
                else:
                    print_fail(f"Тест '{name}' НЕ пройден")
                    failures += 1
            except Exception as e:
                print_fail(f"Тест '{name}' упал с исключением: {e}")
                failures += 1

        self.cleanup()
        print_header("ИТОГ")
        if failures == 0:
            print_ok("Все функциональные тесты успешно пройдены!")
        else:
            print_fail(f"{failures} тест(ов) не пройдено.")
        return 0 if failures == 0 else 1

if __name__ == "__main__":
    tester = SystemTests()
    sys.exit(tester.run_all())