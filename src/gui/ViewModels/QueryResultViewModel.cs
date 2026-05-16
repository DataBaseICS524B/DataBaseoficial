using System.ComponentModel;
using System.Data;
using System.Runtime.CompilerServices;

namespace CustomDB.UI.ViewModels
{
    public class QueryResultViewModel : INotifyPropertyChanged
    {
        private DataTable _resultData = new();
        private string _statusMessage = string.Empty;
        private bool _hasError = false;

        public DataTable ResultData
        {
            get => _resultData;
            set
            {
                _resultData = value;
                OnPropertyChanged();
            }
        }

        public string StatusMessage
        {
            get => _statusMessage;
            set
            {
                _statusMessage = value;
                OnPropertyChanged();
            }
        }

        public bool HasError
        {
            get => _hasError;
            set
            {
                _hasError = value;
                OnPropertyChanged();
            }
        }

        public event PropertyChangedEventHandler? PropertyChanged;

        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        public void Clear()
        {
            ResultData = new DataTable();
            StatusMessage = string.Empty;
            HasError = false;
        }
    }
}
