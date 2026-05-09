using System;
using System.Runtime.InteropServices;

namespace CustomDB.UI.Services
{
    public static class NativeMethods
    {
        // Windows: customdb.dll
        // Linux: libcustomdb.so
        // macOS: libcustomdb.dylib

        private const string DllName = "customdb.dll";

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr db_connect(string host, int port);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr db_execute(IntPtr connection, string query);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void db_disconnect(IntPtr connection);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void db_free_string(IntPtr ptr);
    }
}
