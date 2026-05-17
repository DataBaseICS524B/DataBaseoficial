using System.Windows;
using CustomDB.UI.Services;

namespace CustomDB.UI
{
    public partial class MainWindow : Window
    {
        private DatabaseService _dbService;

        public MainWindow()
        {
            InitializeComponent();
            _dbService = new DatabaseService();

            this.Loaded += MainWindow_Loaded;
        }

        private void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            string host = "localhost";
            int port = 5432;

            string result = _dbService.Connect(host, port);
            if (!_dbService.IsConnected)
            {
                MessageBox.Show($"Не удалось подключиться к серверу: {result}", "Ошибка", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            else
            {

            }
        }

        private void ConnectButton_Click(object sender, RoutedEventArgs e)
        {
            string host = "localhost";
            int port = 5432;
            string result = _dbService.Connect(host, port);
        }
    }
}