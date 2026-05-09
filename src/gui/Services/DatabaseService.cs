using System;
using System.Data;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using Newtonsoft.Json.Linq;

namespace CustomDB.UI.Services
{
    public class QueryResult
    {
        public string Type { get; set; } = string.Empty;
        public List<string> Columns { get; set; } = new();
        public List<List<object>> Rows { get; set; } = new();
        public int AffectedRows { get; set; }
        public string Status { get; set; } = string.Empty;
        public string Error { get; set; } = string.Empty;
    }

    public class DatabaseService
    {
        private IntPtr _connection = IntPtr.Zero;
        private bool _isConnected = false;

        public bool IsConnected => _isConnected;

        public string Connect(string host, int port)
        {
            try
            {
                _connection = NativeMethods.db_connect(host, port);
                if (_connection != IntPtr.Zero)
                {
                    _isConnected = true;
                    return "Connected successfully";
                }
                return "Connection failed";
            }
            catch (Exception ex)
            {
                return $"Connection error: {ex.Message}";
            }
        }

        public QueryResult ExecuteQuery(string query)
        {
            if (!_isConnected || _connection == IntPtr.Zero)
            {
                return new QueryResult { Type = "error", Error = "Not connected to database" };
            }

            try
            {
                IntPtr resultPtr = NativeMethods.db_execute(_connection, query);
                if (resultPtr == IntPtr.Zero)
                {
                    return new QueryResult { Type = "error", Error = "No result returned" };
                }

                string jsonResult = Marshal.PtrToStringUTF8(resultPtr) ?? string.Empty;
                NativeMethods.db_free_string(resultPtr);

                if (string.IsNullOrEmpty(jsonResult))
                {
                    return new QueryResult { Type = "error", Error = "Empty result" };
                }

                return ParseResult(jsonResult);
            }
            catch (Exception ex)
            {
                return new QueryResult { Type = "error", Error = $"Execution error: {ex.Message}" };
            }
        }

        private QueryResult ParseResult(string json)
        {
            try
            {
                var obj = JObject.Parse(json);
                var result = new QueryResult();

                result.Type = obj["type"]?.ToString() ?? "error";

                switch (result.Type)
                {
                    case "select":
                        result.Columns = obj["columns"]?.ToObject<List<string>>() ?? new List<string>();
                        result.Rows = obj["rows"]?.ToObject<List<List<object>>>() ?? new List<List<object>>();
                        break;

                    case "dml":
                        result.AffectedRows = obj["affected_rows"]?.Value<int>() ?? 0;
                        break;

                    case "ddl":
                        result.Status = obj["status"]?.ToString() ?? "OK";
                        break;

                    case "error":
                        result.Error = obj["error"]?.ToString() ?? "Unknown error";
                        break;

                    default:
                        result.Error = $"Unknown result type: {result.Type}";
                        break;
                }

                return result;
            }
            catch (Exception ex)
            {
                return new QueryResult { Type = "error", Error = $"JSON parse error: {ex.Message}" };
            }
        }

        public DataTable ResultToDataTable(QueryResult result)
        {
            var dataTable = new DataTable();

            if (result.Type != "select" || result.Columns.Count == 0)
                return dataTable;

            foreach (var col in result.Columns)
            {
                dataTable.Columns.Add(col, typeof(string));
            }

            foreach (var row in result.Rows)
            {
                var dataRow = dataTable.NewRow();
                for (int i = 0; i < row.Count && i < result.Columns.Count; i++)
                {
                    dataRow[i] = row[i]?.ToString() ?? string.Empty;
                }
                dataTable.Rows.Add(dataRow);
            }

            return dataTable;
        }

        public void Disconnect()
        {
            if (_isConnected && _connection != IntPtr.Zero)
            {
                NativeMethods.db_disconnect(_connection);
                _connection = IntPtr.Zero;
                _isConnected = false;
            }
        }
    }
}
