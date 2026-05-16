using System;
using System.Data;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using System.Collections.Generic;

namespace CustomDB.UI.Services
{
    public class QueryResult
    {
        public string Type { get; set; } = "";
        public List<string> Columns { get; set; } = new();
        public List<List<object>> Rows { get; set; } = new();
        public int AffectedRows { get; set; }
        public string Status { get; set; } = "";
        public string Error { get; set; } = "";
    }

    public class DatabaseService
    {
        private TcpClient? _client;
        private NetworkStream? _stream;
        private bool _isConnected;

        public bool IsConnected => _isConnected;

        public string Connect(string host, int port)
        {
            try
            {
                _client = new TcpClient();
                _client.Connect(host, port);
                _stream = _client.GetStream();
                _isConnected = true;
                return "Connected successfully";
            }
            catch (Exception ex)
            {
                _isConnected = false;
                return $"Connection failed: {ex.Message}";
            }
        }

        public QueryResult ExecuteQuery(string query)
        {
            if (!_isConnected || _stream == null)
                return new QueryResult { Type = "error", Error = "Not connected" };

            try
            {
                byte[] queryBytes = Encoding.UTF8.GetBytes(query);
                byte[] len = BitConverter.GetBytes(queryBytes.Length);
                if (BitConverter.IsLittleEndian) Array.Reverse(len);
                _stream.Write(len, 0, 4);
                _stream.Write(queryBytes, 0, queryBytes.Length);

                byte[] status = new byte[4];
                _stream.Read(status, 0, 4);

                byte[] dataLenBytes = new byte[4];
                _stream.Read(dataLenBytes, 0, 4);
                if (BitConverter.IsLittleEndian) Array.Reverse(dataLenBytes);
                int dataLen = BitConverter.ToInt32(dataLenBytes, 0);

                byte[] response = new byte[dataLen];
                int received = 0;
                while (received < dataLen)
                    received += _stream.Read(response, received, dataLen - received);

                string json = Encoding.UTF8.GetString(response);
                return ParseResult(json);
            }
            catch (Exception ex)
            {
                return new QueryResult { Type = "error", Error = ex.Message };
            }
        }

        private QueryResult ParseResult(string json)
        {
            try
            {
                using var doc = JsonDocument.Parse(json);
                var root = doc.RootElement;
                string type = root.GetProperty("type").GetString() ?? "error";
                var result = new QueryResult { Type = type };

                if (type == "select")
                {
                    if (root.TryGetProperty("columns", out var cols))
                        foreach (var c in cols.EnumerateArray())
                            result.Columns.Add(c.GetString() ?? "");
                    if (root.TryGetProperty("rows", out var rows))
                        foreach (var row in rows.EnumerateArray())
                        {
                            var rowList = new List<object>();
                            foreach (var val in row.EnumerateArray())
                                rowList.Add(val.GetString() ?? "");
                            result.Rows.Add(rowList);
                        }
                }
                else if (type == "dml")
                {
                    result.AffectedRows = root.GetProperty("affected_rows").GetInt32();
                }
                else if (type == "ddl")
                {
                    result.Status = root.GetProperty("message").GetString() ?? "OK";
                }
                else if (type == "error")
                {
                    result.Error = root.GetProperty("message").GetString() ?? "Unknown error";
                }
                return result;
            }
            catch (Exception ex)
            {
                return new QueryResult { Type = "error", Error = $"JSON parse: {ex.Message}" };
            }
        }

        public DataTable ResultToDataTable(QueryResult result)
        {
            var dt = new DataTable();
            if (result.Type != "select") return dt;
            foreach (var col in result.Columns) dt.Columns.Add(col, typeof(string));
            foreach (var row in result.Rows)
            {
                var dr = dt.NewRow();
                for (int i = 0; i < row.Count && i < result.Columns.Count; i++)
                    dr[i] = row[i]?.ToString() ?? "";
                dt.Rows.Add(dr);
            }
            return dt;
        }

        public void Disconnect()
        {
            _stream?.Close();
            _client?.Close();
            _isConnected = false;
        }
    }
}