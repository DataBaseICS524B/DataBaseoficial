using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Data;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Windows;
using CustomDB.UI.Services;

namespace CustomDB.UI.ViewModels
{
    public class RelayCommand : ICommand
    {
        private readonly Action<object?> _execute;
        private readonly Func<object?, bool>? _canExecute;
        private event EventHandler? _canExecuteChanged;

        public RelayCommand(Action<object?> execute, Func<object?, bool>? canExecute = null)
        {
            _execute = execute ?? throw new ArgumentNullException(nameof(execute));
            _canExecute = canExecute;
        }

        public bool CanExecute(object? parameter) => _canExecute == null || _canExecute(parameter);

        public void Execute(object? parameter) => _execute(parameter);

        public event EventHandler? CanExecuteChanged
        {
            add
            {
                _canExecuteChanged += value;
                CommandManager.RequerySuggested += value;
            }
            remove
            {
                _canExecuteChanged -= value;
                CommandManager.RequerySuggested -= value;
            }
        }

        public void RaiseCanExecuteChanged()
        {
            _canExecuteChanged?.Invoke(this, EventArgs.Empty);
        }
    }

    public class MainViewModel : INotifyPropertyChanged
    {
        private readonly DatabaseService _dbService;
        private string _host = "localhost";
        private int _port = 5432;
        private string _query = string.Empty;
        private string _connectionStatus = "Disconnected";
        private bool _isConnected = false;
        private bool _isBusy = false;
        private QueryResultViewModel _queryResult = new();

        private RelayCommand? _connectCommand;
        private RelayCommand? _disconnectCommand;
        private RelayCommand? _executeCommand;
        private RelayCommand? _clearCommand;

        public MainViewModel()
        {
            _dbService = new DatabaseService();
        }

        public string Host
        {
            get => _host;
            set { _host = value; OnPropertyChanged(); }
        }

        public int Port
        {
            get => _port;
            set { _port = value; OnPropertyChanged(); }
        }

        public string Query
        {
            get => _query;
            set { _query = value; OnPropertyChanged(); }
        }

        public string ConnectionStatus
        {
            get => _connectionStatus;
            set { _connectionStatus = value; OnPropertyChanged(); }
        }

        public bool IsConnected
        {
            get => _isConnected;
            set
            {
                _isConnected = value;
                OnPropertyChanged();
                _connectCommand?.RaiseCanExecuteChanged();
                _disconnectCommand?.RaiseCanExecuteChanged();
                _executeCommand?.RaiseCanExecuteChanged();
            }
        }

        public bool IsBusy
        {
            get => _isBusy;
            set
            {
                _isBusy = value;
                OnPropertyChanged();
                _connectCommand?.RaiseCanExecuteChanged();
                _disconnectCommand?.RaiseCanExecuteChanged();
                _executeCommand?.RaiseCanExecuteChanged();
            }
        }

        public QueryResultViewModel QueryResult
        {
            get => _queryResult;
            set { _queryResult = value; OnPropertyChanged(); }
        }

        public ObservableCollection<string> QueryHistory { get; } = new();

        public ICommand ConnectCommand => _connectCommand ??= new RelayCommand(_ => Connect(), _ => !IsBusy && !IsConnected);
        public ICommand DisconnectCommand => _disconnectCommand ??= new RelayCommand(_ => Disconnect(), _ => !IsBusy && IsConnected);
        public ICommand ExecuteCommand => _executeCommand ??= new RelayCommand(_ => ExecuteQuery(), _ => !IsBusy && IsConnected && !string.IsNullOrWhiteSpace(Query));
        public ICommand ClearCommand => _clearCommand ??= new RelayCommand(_ => Clear());

        private async void Connect()
        {
            IsBusy = true;
            ConnectionStatus = "Connecting...";
            await Task.Delay(100);
            string result = await Task.Run(() => _dbService.Connect(Host, Port));
            Application.Current.Dispatcher.Invoke(() =>
            {
                if (_dbService.IsConnected)
                {
                    IsConnected = true;
                    ConnectionStatus = $"Connected to {Host}:{Port}";
                }
                else
                {
                    IsConnected = false;
                    ConnectionStatus = $"Failed: {result}";
                    QueryResult.StatusMessage = result;
                    QueryResult.HasError = true;
                }
                IsBusy = false;
            });
        }

        private void Disconnect()
        {
            _dbService.Disconnect();
            IsConnected = false;
            ConnectionStatus = "Disconnected";
            QueryResult.Clear();
        }

        private async void ExecuteQuery()
        {
            if (string.IsNullOrWhiteSpace(Query))
                return;

            IsBusy = true;
            QueryResult.Clear();
            QueryResult.StatusMessage = "Executing...";

            var execResult = await Task.Run(() => _dbService.ExecuteQuery(Query));

            Application.Current.Dispatcher.Invoke(() =>
            {
                ProcessResult(execResult);
                // Добавляем в историю (опционально, без сохранения в Settings)
                if (!QueryHistory.Contains(Query))
                {
                    QueryHistory.Insert(0, Query);
                    if (QueryHistory.Count > 20)
                        QueryHistory.RemoveAt(QueryHistory.Count - 1);
                }
                IsBusy = false;
            });
        }

        private void ProcessResult(QueryResult result)
        {
            switch (result.Type)
            {
                case "select":
                    var dataTable = _dbService.ResultToDataTable(result);
                    QueryResult.ResultData = dataTable;
                    QueryResult.StatusMessage = $"Selected {dataTable.Rows.Count} row(s)";
                    QueryResult.HasError = false;
                    break;
                case "dml":
                    QueryResult.ResultData = new DataTable();
                    QueryResult.StatusMessage = $"Affected rows: {result.AffectedRows}";
                    QueryResult.HasError = false;
                    break;
                case "ddl":
                    QueryResult.ResultData = new DataTable();
                    QueryResult.StatusMessage = $"OK - {result.Status}";
                    QueryResult.HasError = false;
                    break;
                case "error":
                    QueryResult.ResultData = new DataTable();
                    QueryResult.StatusMessage = $"Error: {result.Error}";
                    QueryResult.HasError = true;
                    break;
                default:
                    QueryResult.ResultData = new DataTable();
                    QueryResult.StatusMessage = $"Unknown result type: {result.Type}";
                    QueryResult.HasError = true;
                    break;
            }
        }

        private void Clear()
        {
            Query = string.Empty;
            QueryResult.Clear();
        }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }
}