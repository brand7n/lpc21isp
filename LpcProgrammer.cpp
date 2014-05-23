#include "LpcProgrammer.h"

using FTD2XX_NET::FTDI;
namespace lpcprog
{

LpcProgrammer::LpcProgrammer(StringOutDelegate ^fp)
{
	Callbacks::myFp= fp;
}

int LpcProgrammer::Program(System::String ^firmwareName)
{
	char strOscStorage[] = "12000";
	char baudStorage[] = "230400";
	char serialStorage[] = "\\\\.\\COM99";
	// set up ISP_ENVIRONMENT from original code, now a managed struct
	IspEnvironment.f_list = NULL; // XXX leak
	IspEnvironment.StringOscillator = strOscStorage; // done this way to fix conversion to C++/CLI
	IspEnvironment.baud_rate = baudStorage; // XXX ignored!
	IspEnvironment.serial_port = serialStorage;

	if (OpenFTDI() < 0)
	{
		return -1;
	}

    // Initialize debug level
    debug_level = 2;

    // Initialize ISP Environment
    //memset(&IspEnvironment, 0, sizeof(IspEnvironment));       // Clear the IspEnviroment to a known value
    IspEnvironment.micro       = NXP_ARM;                     // Default Micro
    IspEnvironment.FileFormat  = FORMAT_HEX;                  // Default File Format
    IspEnvironment.ProgramChip = TRUE;                        // Default to Programming the chip
    IspEnvironment.nQuestionMarks = 100;
    IspEnvironment.DoNotStart = 1;
	IspEnvironment.WipeDevice = 1;
	IspEnvironment.Verify = 0;
	IspEnvironment.ControlLines = 1;

	array<System::Byte> ^chars = System::Text::Encoding::ASCII->GetBytes(firmwareName);
	pin_ptr<System::Byte> charsPointer = &(chars[0]);
	char *nativeCharsPointer = reinterpret_cast<char *>(static_cast<unsigned char *>(charsPointer));
	AddFileHex(%IspEnvironment, nativeCharsPointer);
	
    return PerformActions(%IspEnvironment);                   // Do as requested !
}

int LpcProgrammer::OpenFTDI(void)
{
	unsigned int ftdiDeviceCount;
    FTDI::FT_STATUS ftStatus = FTDI::FT_STATUS::FT_OK;

    // Determine the number of FTDI devices connected to the machine
    ftStatus = IspEnvironment.myFtdiDevice.GetNumberOfDevices(ftdiDeviceCount);
    // Check status
    if (ftStatus != FTDI::FT_STATUS::FT_OK)
    {
		return -1;
    }

    // Populate our device list
	array<FTDI::FT_DEVICE_INFO_NODE ^> ^deviceList = gcnew array<FTDI::FT_DEVICE_INFO_NODE ^>(ftdiDeviceCount);
    ftStatus = IspEnvironment.myFtdiDevice.GetDeviceList(deviceList);
	if (ftStatus != FTDI::FT_STATUS::FT_OK)
    {
		return -1;
    }

	int oIdx = -1;
	for (int i=0; i<ftdiDeviceCount; i++)
	{
		if (deviceList[i]->Type == FTDI::FT_DEVICE::FT_DEVICE_232R)
        {
			oIdx = i;
        }
	}

	if (oIdx < 0)
	{
		return -1;
	}

	ftStatus = IspEnvironment.myFtdiDevice.OpenBySerialNumber(deviceList[oIdx]->SerialNumber);
	if (ftStatus != FTDI::FT_STATUS::FT_OK)
    {
		return -1;
    }

	System::String^ comName;
	IspEnvironment.myFtdiDevice.GetCOMPort(comName);
	array<System::Byte> ^chars = System::Text::Encoding::ASCII->GetBytes(comName);
	pin_ptr<System::Byte> charsPointer = &(chars[0]);
	char *nativeCharsPointer = reinterpret_cast<char *>(static_cast<unsigned char *>(charsPointer));
    strcpy(IspEnvironment.serial_port+4, nativeCharsPointer);

	IspEnvironment.myFtdiDevice.Close();
	return 0; // XXX for now return here and pass COM port to lpc21isp

	IspEnvironment.myFtdiDevice.SetDTR(false);
    IspEnvironment.myFtdiDevice.SetRTS(false);

    // Set up device data parameters
    ftStatus = IspEnvironment.myFtdiDevice.SetBaudRate(230400);
    if (ftStatus != FTDI::FT_STATUS::FT_OK)
    {
        // Wait for a key press
        //Console.WriteLine("Failed to set Baud rate (error " + ftStatus.ToString() + ")");
        //Console.ReadKey();
        return -1;
    }

    // Set data characteristics - Data bits, Stop bits, Parity
    ftStatus = IspEnvironment.myFtdiDevice.SetDataCharacteristics(FTDI::FT_DATA_BITS::FT_BITS_8, FTDI::FT_STOP_BITS::FT_STOP_BITS_1, FTDI::FT_PARITY::FT_PARITY_NONE);
    if (ftStatus != FTDI::FT_STATUS::FT_OK)
    {
        // Wait for a key press
        //Console.WriteLine("Failed to set data characteristics (error " + ftStatus.ToString() + ")");
        //Console.ReadKey();
        return -1;
    }

    // Set flow control - set RTS/CTS flow control
    ftStatus = IspEnvironment.myFtdiDevice.SetFlowControl(FTDI::FT_FLOW_CONTROL::FT_FLOW_NONE, 0x11, 0x13);
    if (ftStatus != FTDI::FT_STATUS::FT_OK)
    {
        // Wait for a key press
        //Console.WriteLine("Failed to set flow control (error " + ftStatus.ToString() + ")");
        //Console.ReadKey();
        return -1;
    }

    // times out at 100ms
    ftStatus = IspEnvironment.myFtdiDevice.SetTimeouts(500, 500);
    if (ftStatus != FTDI::FT_STATUS::FT_OK)
    {
        // Wait for a key press
        //Console.WriteLine("Failed to set timeouts (error " + ftStatus.ToString() + ")");
        //Console.ReadKey();
        return -1;
    }
	return 0;
}

}
