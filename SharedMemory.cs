using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace ShareMemery
{
    public class ShareMemLib
    {
        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        public static extern IntPtr SendMessage(IntPtr hWnd, int Msg, int wParam, IntPtr lParam);

        [DllImport("Kernel32.dll", CharSet = CharSet.Auto)]
        public static extern IntPtr CreateFileMapping(int hFile, IntPtr lpAttributes, uint flProtect, uint dwMaxSizeHi, uint dwMaxSizeLow, string lpName);

        [DllImport("Kernel32.dll", CharSet = CharSet.Auto)]
        public static extern IntPtr OpenFileMapping(int dwDesiredAccess, [MarshalAs(UnmanagedType.Bool)] bool bInheritHandle, string lpName);

        [DllImport("Kernel32.dll", CharSet = CharSet.Auto)]
        public static extern IntPtr MapViewOfFile(IntPtr hFileMapping, uint dwDesiredAccess, uint dwFileOffsetHigh, uint dwFileOffsetLow, uint dwNumberOfBytesToMap);

        [DllImport("Kernel32.dll", CharSet = CharSet.Auto)]
        public static extern bool UnmapViewOfFile(IntPtr pvBaseAddress);

        [DllImport("Kernel32.dll", CharSet = CharSet.Auto)]
        public static extern bool CloseHandle(IntPtr handle);

        [DllImport("kernel32", EntryPoint = "GetLastError")]
        public static extern int GetLastError();

        const int ERROR_ALREADY_EXISTS = 183;

        const int FILE_MAP_COPY = 0x0001;
        const int FILE_MAP_WRITE = 0x0002;
        const int FILE_MAP_READ = 0x0004;
        const int FILE_MAP_ALL_ACCESS = 0x0002 | 0x0004;

        const int PAGE_READONLY = 0x02;
        const int PAGE_READWRITE = 0x04;
        const int PAGE_WRITECOPY = 0x08;
        const int PAGE_EXECUTE = 0x10;
        const int PAGE_EXECUTE_READ = 0x20;
        const int PAGE_EXECUTE_READWRITE = 0x40;

        const int SEC_COMMIT = 0x8000000;
        const int SEC_IMAGE = 0x1000000;
        const int SEC_NOCACHE = 0x10000000;
        const int SEC_RESERVE = 0x4000000;

        const int INVALID_HANDLE_VALUE = -1;

        IntPtr m_hSharedMemoryFile = IntPtr.Zero;
        IntPtr m_pwData = IntPtr.Zero;
        bool m_bAlreadyExist = false;
        bool m_bInit = false;
        long m_MemSize = 0;

        public ShareMemLib()
        {
        }
        ~ShareMemLib()
        {
            Close();
        }

        /// <summary>
        /// Init Shared Memory
        /// </summary>
        /// <param name="strName">Name of the shared memory</param>
        /// <param name="lngSize">Size of the shared memory</param>
        /// <returns></returns>
        public int Init(string strName, long lngSize)
        {
            if (lngSize <= 0 || lngSize > 0x00800000) lngSize = 0x00800000;
            m_MemSize = lngSize;
            if (strName.Length > 0)
            {
                //Create shared memory pagefile
                m_hSharedMemoryFile = CreateFileMapping(INVALID_HANDLE_VALUE, IntPtr.Zero, (uint)PAGE_READWRITE, 0, (uint)lngSize, strName);
                if (m_hSharedMemoryFile == IntPtr.Zero)
                {
                    m_bAlreadyExist = false;
                    m_bInit = false;
                    return 2; //Failed to create shared memory pagefile
                }
                else
                {
                    if (GetLastError() == ERROR_ALREADY_EXISTS)  //Already exist
                    {
                        m_bAlreadyExist = true;
                    }
                    else                                         //New
                    {
                        m_bAlreadyExist = false;
                    }
                }
                //---------------------------------------
                //Create shared memory map
                m_pwData = MapViewOfFile(m_hSharedMemoryFile, FILE_MAP_WRITE, 0, 0, (uint)lngSize);
                if (m_pwData == IntPtr.Zero)
                {
                    m_bInit = false;
                    CloseHandle(m_hSharedMemoryFile);
                    return 3; //Failed to create shared memory map
                }
                else
                {
                    m_bInit = true;
                    if (m_bAlreadyExist == false)
                    {
                        //Init
                    }
                }
                //----------------------------------------
            }
            else
            {
                return 1; //Wrong arguments     
            }

            return 0;     //Create successful
        }
        /// <summary>
        /// Close shared memory
        /// </summary>
        public void Close()
        {
            if (m_bInit)
            {
                UnmapViewOfFile(m_pwData);
                CloseHandle(m_hSharedMemoryFile);
            }
        }

        /// <summary>
        /// Read data
        /// </summary>
        /// <param name="bytData">Data</param>
        /// <param name="lngAddr">Address of the data</param>
        /// <param name="lngSize">Size of the data</param>
        /// <returns></returns>
        public int Read(ref byte[] bytData, int lngAddr, int lngSize)
        {
            if (lngAddr + lngSize > m_MemSize) return 2; //Length exceeded
            if (m_bInit)
            {
                Marshal.Copy(m_pwData, bytData, lngAddr, lngSize);
            }
            else
            {
                return 1; //Shared memory has not initializeed
            }
            return 0;     //Read Successful
        }

        /// <summary>
        /// Write data
        /// </summary>
        /// <param name="bytData">Data</param>
        /// <param name="lngAddr">Address of the data</param>
        /// <param name="lngSize">Size of the data</param>
        /// <returns></returns>
        public int Write(byte[] bytData, int lngAddr, int lngSize)
        {
            if (lngAddr + lngSize > m_MemSize) return 2; //Length exceeded
            if (m_bInit)
            {
                Marshal.Copy(bytData, lngAddr, m_pwData, lngSize);
            }
            else
            {
                return 1; //Shared memory has not initializeed
            }
            return 0;     //Write successful
        }

        public static byte[] StructToBytes(object structObj, int size = 0)
        {
            if (size == 0)
            {
                size = Marshal.SizeOf(structObj); //Calc the size of the structure
            }
            IntPtr buffer = Marshal.AllocHGlobal(size);  //Allocate memory
            try
            {
                Marshal.StructureToPtr(structObj, buffer, false);   //Fill memory space
                byte[] bytes = new byte[size];
                Marshal.Copy(buffer, bytes, 0, size);   //Fill the array
                return bytes;
            }
            catch (Exception ex)
            {
                return null;
            }
            finally
            {
                Marshal.FreeHGlobal(buffer);    //Free memory
            }
        }

        public static object BytesToStruct(byte[] bytes, Type strcutType, int nSize)
        {
            if (bytes == null)
            {
                return null;
            }
            int size = Marshal.SizeOf(strcutType);
            IntPtr buffer = Marshal.AllocHGlobal(nSize);
            try
            {
                Marshal.Copy(bytes, 0, buffer, nSize);
                return Marshal.PtrToStructure(buffer, strcutType);
            }
            catch (Exception ex)
            {
                return null;
            }
            finally
            {
                Marshal.FreeHGlobal(buffer);    //Free memory
            }
        }

        public int Read(ref object input, Type strcutType)
        {
            int length = Marshal.SizeOf(strcutType);
            byte[] bytData = new byte[length];
            int error = Read(ref bytData, 0, length);
            input = BytesToStruct(bytData, strcutType, length);
            return error;
        }

        public int Write(ref object output, Type strcutType)
        {
            int length = Marshal.SizeOf(strcutType);
            byte[] bytdata = StructToBytes(output, length);
            int error = Write(bytdata, 0, length);
            return error;
        }
    }
}