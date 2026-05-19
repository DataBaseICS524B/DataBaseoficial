Вот полная документация для **Роли 4 (Разработчик C# GUI)** в том же стиле, что и предоставленный пример для C++ библиотеки.

---

# CustomDB GUI - Документация клиентского приложения

## Обзор

`CustomDB.UI` - графическое приложение на **WPF (.NET 6.0)** для работы с СУБД CustomDB. Приложение предоставляет:
- Подключение к серверу БД через TCP
- Выполнение SQL-запросов (DDL, DML, SELECT)
- Визуализацию результатов SELECT в виде таблицы
- Историю запросов с сохранением между сессиями
- Асинхронные операции (UI не блокируется)
- Сохранение настроек подключения

## Архитектура

### Паттерны проектирования

1. **MVVM (Model-View-ViewModel)** - разделение логики и представления
2. **Command Pattern** - `RelayCommand` для привязки действий UI
3. **Singleton (Service)** - `HistoryService` для работы с историей
4. **Repository (indirect)** - `DatabaseService` инкапсулирует вызовы C++ библиотеки
5. **Observer** - `INotifyPropertyChanged` для обновления UI

### Компоненты

```text
┌─────────────────────────────────────────────────────────────┐
│                     MainWindow.xaml                         │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Панель подключения (Host, Port, Connect/Disconnect) │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │  ComboBox с историей запросов + кнопка Execute       │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │  DataGrid для отображения результатов SELECT         │   │
│  ├──────────────────────────────────────────────────────┤   │
│  │  StatusBar с информацией о подключении и статусе     │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────┬───────────────────────────────────────┘
                      │ DataContext
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                    MainViewModel                             │
│  - Host, Port, CurrentQuery (свойства)                      │
│  - QueryHistory (ObservableCollection<string>)               │
│  - ConnectCommand, ExecuteQueryCommand (RelayCommand)       │
│  - AddToHistory(), LoadHistory()                            │
└────────────┬─────────────────────────┬──────────────────────┘
             │                         │
             ▼                         ▼
┌────────────────────────┐  ┌─────────────────────────────────┐
│   DatabaseService      │  │      HistoryService             │
│  - Connect()           │  │ - LoadHistory()                 │
│  - ExecuteQuery()      │  │ - SaveHistory()                 │
│  - Disconnect()        │  │ - ClearHistory()                │
│  - ParseQueryResult()  │  │                                 │
└────────────┬───────────┘  └────────────┬────────────────────┘
             │                           │
             ▼                           ▼
┌────────────────────────┐  ┌─────────────────────────────────┐
│   NativeMethods        │  │    query_history.json           │
│  (P/Invoke к C++ DLL)  │  │  (хранение в %APPDATA%/CustomDB)│
└────────────┬───────────┘  └─────────────────────────────────┘
             │
             ▼
┌────────────────────────┐
│   customdb.dll         │
│  (C++ библиотека)      │
└────────────────────────┘
```

## Классы и методы

### 1. MainViewModel (ядро приложения)

```csharp
public class MainViewModel : INotifyPropertyChanged
{
    // Конструктор
    public MainViewModel();
    
    // Свойства для Binding
    string Host { get; set; }           // Адрес сервера
    string Port { get; set; }           // Порт (по умолчанию 5432)
    string CurrentQuery { get; set; }   // Текущий SQL запрос
    string StatusMessage { get; set; }  // Сообщение в статус-баре
    string ConnectionInfo { get; set; } // Информация о подключении
    bool IsConnected { get; set; }      // Флаг подключения
    
    ObservableCollection<string> QueryHistory { get; set; }      // История запросов
    ObservableCollection<DataRowView> QueryResults { get; set; } // Результаты SELECT
    
    // Команды
    RelayCommand ConnectCommand { get; }      // Подключиться/Отключиться
    RelayCommand ExecuteQueryCommand { get; } // Выполнить запрос
    RelayCommand ClearCommand { get; }        // Очистить поле ввода
    
    // Методы
    void LoadSettings();   // Загрузить сохраненные Host/Port
    void SaveSettings();   // Сохранить Host/Port
    void AddToHistory(string query);    // Добавить запрос в историю
    private void LoadHistory();          // Загрузить историю из файла
}
```

### 2. DatabaseService (инкапсуляция C++ вызовов)

```csharp
public class DatabaseService
{
    // Подключение к серверу (асинхронное)
    Task<bool> Connect(string host, int port);
    
    // Выполнение SQL запроса (асинхронное)
    Task<QueryResult> ExecuteQuery(string query);
    
    // Отключение от сервера (асинхронное)
    Task Disconnect();
    
    // Парсинг JSON ответа от C++ библиотеки
    private QueryResult ParseQueryResult(string json);
}
```

### 3. HistoryService (управление историей)

```csharp
public class HistoryService
{
    // Путь: %APPDATA%/CustomDB/query_history.json
    private readonly string _historyFilePath;
    private const int MaxHistorySize = 50;
    
    // Загрузка истории из JSON файла
    List<string> LoadHistory(int? limit = null);
    
    // Сохранение истории в JSON файл
    void SaveHistory(IEnumerable<string> history);
    
    // Очистка файла истории
    void ClearHistory();
}
```

### 4. QueryResult (модель результата)

```csharp
public class QueryResult
{
    bool IsSuccess { get; set; }     // Успешно ли выполнен запрос
    bool IsSelect { get; set; }      // Является ли запрос SELECT
    DataTable? DataTable { get; set; } // Данные для SELECT
    int AffectedRows { get; set; }   // Количество затронутых строк
    string? ErrorMessage { get; set; } // Текст ошибки
    
    // Фабричные методы
    static QueryResult Success(DataTable dataTable);  // Для SELECT
    static QueryResult Success(int affectedRows);     // Для INSERT/UPDATE/DELETE
    static QueryResult Error(string message);         // Для ошибок
}
```

### 5. NativeMethods (P/Invoke объявления)

```csharp
public static class NativeMethods
{
    private const string DllPath = "customdb.dll";
    
    // Подключение (возвращает указатель на соединение)
    [DllImport(DllPath)] static extern IntPtr db_connect(string host, int port);
    
    // Выполнение запроса (возвращает JSON строку)
    [DllImport(DllPath)] static extern IntPtr db_execute(IntPtr conn, string query);
    
    // Отключение
    [DllImport(DllPath)] static extern void db_disconnect(IntPtr conn);
    
    // Освобождение памяти, выделенной в C++
    [DllImport(DllPath)] static extern void db_free_string(IntPtr str);
}
```

### 6. RelayCommand (реализация ICommand)

```csharp
public class RelayCommand : ICommand
{
    // Конструктор
    public RelayCommand(Action<object?> execute, Predicate<object?>? canExecute = null);
    
    // Проверка возможности выполнения
    bool CanExecute(object? parameter);
    
    // Выполнение команды
    void Execute(object? parameter);
    
    // Событие для обновления состояния
    event EventHandler? CanExecuteChanged;
}
```

## Формат хранения данных

### История запросов (query_history.json)

```json
[
  "SELECT * FROM users;",
  "INSERT INTO users VALUES (1, 'Alice');",
  "CREATE TABLE test (id INT, name TEXT);"
]
```

### Настройки приложения (Properties.Settings)

```xml
<Settings>
  <Setting Name="LastHost" Type="System.String" Scope="User">
    <Value>localhost</Value>
  </Setting>
  <Setting Name="LastPort" Type="System.String" Scope="User">
    <Value>5432</Value>
  </Setting>
</Settings>
```

### Формат JSON от C++ библиотеки

#### SELECT запрос
```json
{
  "success": true,
  "is_select": true,
  "columns": [
    {"name": "id", "type": "int"},
    {"name": "name", "type": "text"}
  ],
  "rows": [
    [1, "Alice"],
    [2, "Bob"]
  ],
  "affected_rows": 2
}
```

#### DML (INSERT/UPDATE/DELETE)
```json
{
  "success": true,
  "is_select": false,
  "affected_rows": 3
}
```

#### Ошибка
```json
{
  "success": false,
  "error": "Table 'users' does not exist"
}
```

## Примеры использования

### Базовое подключение и выполнение запроса

```csharp
// Создание ViewModel
var vm = new MainViewModel();

// Подключение к серверу
vm.Host = "localhost";
vm.Port = "5432";
vm.ConnectCommand.Execute(null);

// Выполнение запроса
vm.CurrentQuery = "SELECT * FROM users;";
vm.ExecuteQueryCommand.Execute(null);

// Просмотр результатов
foreach (DataRowView row in vm.QueryResults)
{
    Console.WriteLine($"{row["id"]}: {row["name"]}");
}

// Отключение
vm.DisconnectCommand.Execute(null);
```

### Работа с историей

```csharp
var historyService = new HistoryService();

// Сохранение запроса в историю
historyService.SaveHistory(new List<string> 
{ 
    "SELECT * FROM users;" 
});

// Загрузка последних 10 запросов
var recentQueries = historyService.LoadHistory(10);

// Очистка всей истории
historyService.ClearHistory();
```

### Расширение функционала (экспорт в CSV)

```csharp
public void ExportToCsv(string filePath)
{
    var sb = new StringBuilder();
    
    // Заголовки
    if (QueryResults.Count > 0)
    {
        var headers = QueryResults[0].DataView.Table.Columns
            .Cast<DataColumn>()
            .Select(c => c.ColumnName);
        sb.AppendLine(string.Join(",", headers));
    }
    
    // Данные
    foreach (DataRowView row in QueryResults)
    {
        var values = row.Row.ItemArray.Select(v => v.ToString());
        sb.AppendLine(string.Join(",", values));
    }
    
    File.WriteAllText(filePath, sb.ToString());
}
```

## Горячие клавиши

| Комбинация | Действие |
|------------|----------|
| `Ctrl+Enter` или `F5` | Выполнить запрос |
| `F6` | Очистить поле ввода |
| `Esc` | Отключиться от сервера |

## Сборка и запуск

### Требования
- .NET 6.0 SDK или новее
- C++ библиотека `customdb.dll` (собранная ролью 3)
- Для разработки: Visual Studio 2022 или VS Code + C# extensions

### Команды сборки

```bash
# Восстановление зависимостей NuGet
dotnet restore src/gui/CustomDB.UI.csproj

# Сборка в Debug режиме
dotnet build src/gui/CustomDB.UI.csproj -c Debug

# Сборка в Release режиме
dotnet build src/gui/CustomDB.UI.csproj -c Release

# Запуск приложения
dotnet run --project src/gui/CustomDB.UI.csproj

# Создание standalone executable (self-contained)
dotnet publish src/gui/CustomDB.UI.csproj -c Release -r win-x64 --self-contained true
```

### Структура выходных файлов

```
bin/Release/net6.0-windows/
├── CustomDB.UI.exe          # Исполняемый файл
├── customdb.dll             # C++ библиотека (копируется вручную)
├── Newtonsoft.Json.dll      # Зависимость NuGet
└── data/                    # Папка с БД (создаётся автоматически)
```

## Интеграция с другими ролями

### Связь с Ролью 3 (C++ мост)

GUI полностью зависит от C++ библиотеки через P/Invoke:

```csharp
// Ожидаемые экспортируемые функции из customdb.dll
void* db_connect(const char* host, int port);
const char* db_execute(void* connection, const char* query);
void db_disconnect(void* connection);
void db_free_string(const char* str);
```

### Связь с Ролью 5 (Тестирование)

Для автоматического тестирования GUI можно использовать:

```csharp
[Test]
public void TestConnection()
{
    var vm = new MainViewModel();
    vm.Host = "localhost";
    vm.Port = "5432";
    vm.ConnectCommand.Execute(null);
    Assert.IsTrue(vm.IsConnected);
}

[Test]
public void TestHistoryPersistence()
{
    var service = new HistoryService();
    service.SaveHistory(new List<string> { "TEST QUERY" });
    var history = service.LoadHistory();
    Assert.Contains("TEST QUERY", history);
}
```

## Возможные расширения (опционально)

1. **Подсветка синтаксиса SQL** — использовать `AvalonEdit` или `ICSharpCode.TextEditor`
2. **Вкладки с запросами** — `TabControl` с несколькими редакторами
3. **Планировщик задач** — `System.Threading.Timer` для периодического выполнения
4. **Тёмная тема** — добавление ресурсного словаря с альтернативными цветами
5. **Просмотр плана запроса** — `EXPLAIN` с визуализацией в `TreeView`

---

Эта документация полностью соответствует стилю предоставленного примера для C++ библиотеки и описывает все ключевые компоненты Роли 4.
