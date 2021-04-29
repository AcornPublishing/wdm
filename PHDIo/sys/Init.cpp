//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	PHDIo example
/////////////////////////////////////////////////////////////////////////////
//	init.cpp:		Driver initialization code
/////////////////////////////////////////////////////////////////////////////
//	DriverEntry		Initialisation entry point
//	PHDIoCreateDevice
//	PHDIoUnload		Unload driver routine
//	PHDIoDeleteDevice
//	LockDevice
//	UnlockDevice
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
//	11-May-99	1.0.3	CC	Stored command buffers freed
//	14-May-99	1.0.3	CC	ConnectIntWQI and ConnectIntQueued initialised
//	20-May-99	1.0.4	CC	Stop timer when device deleted.
/////////////////////////////////////////////////////////////////////////////

#include "PHDIo.h"

/////////////////////////////////////////////////////////////////////////////

PDEVICE_OBJECT phddo = NULL;

NTSTATUS PHDIoCreateDevice( IN PDRIVER_OBJECT DriverObject);
void PHDIoDeleteDevice();

/////////////////////////////////////////////////////////////////////////////

#pragma code_seg("INIT") // start INIT section

/////////////////////////////////////////////////////////////////////////////
//	DriverEntry:
//
//	Description:
//		This function initializes the driver, and creates
//		any objects needed to process I/O requests.
//
//	Arguments:
//		Pointer to the Driver object
//		Registry path string for driver service key
//
//	Return Value:
//		This function returns STATUS_XXX

extern "C"
NTSTATUS DriverEntry(	IN PDRIVER_OBJECT DriverObject,
						IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

#if DBG
	DebugPrintInit("PHDIo checked");
#else
	DebugPrintInit("PHDIo free");
#endif

	DebugPrint("RegistryPath is %T",RegistryPath);

	// Export other driver entry points...
// 	DriverObject->DriverExtension->AddDevice = PHDIoAddDevice;
 	DriverObject->DriverStartIo = PHDIoStartIo;
 	DriverObject->DriverUnload = PHDIoUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = PHDIoCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = PHDIoClose;

//	DriverObject->MajorFunction[IRP_MJ_PNP] = PHDIoPnp;		No PnP handler...
//	DriverObject->MajorFunction[IRP_MJ_POWER] = PHDIoPower;

	DriverObject->MajorFunction[IRP_MJ_READ] = PHDIoRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = PHDIoWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PHDIoDeviceControl;

//	DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PHDIoSystemControl;

	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = PHDIoDispatchCleanup;

	status = PHDIoCreateDevice(DriverObject);

	DebugPrintMsg("DriverEntry completed");

	return status;
}

//////////////////////////////////////////////////////////////////////////////

#define	NT_DEVICE_NAME	L"\\Device\\PHDIo"
#define	SYM_LINK_NAME	L"\\DosDevices\\PHDIo"

NTSTATUS PHDIoCreateDevice( IN PDRIVER_OBJECT DriverObject)
{
	NTSTATUS status = STATUS_SUCCESS;

	// Initialise NT and Symbolic link names
	UNICODE_STRING deviceName, linkName;
	RtlInitUnicodeString( &deviceName, NT_DEVICE_NAME);
	RtlInitUnicodeString( &linkName, SYM_LINK_NAME);

	// Create our device
	DebugPrint("Creating device %T",&deviceName);
	status = IoCreateDevice(
				DriverObject,
				sizeof(PHDIO_DEVICE_EXTENSION),
				&deviceName,
				FILE_DEVICE_UNKNOWN,
				0,
				TRUE,	// Exclusive
				&phddo);
	if( !NT_SUCCESS(status))
	{
		DebugPrintMsg("Could not create device");
		return status;
	}

	phddo->Flags |= DO_BUFFERED_IO;

	// Initialise device extension
	PPHDIO_DEVICE_EXTENSION dx = (PPHDIO_DEVICE_EXTENSION)phddo->DeviceExtension;
	dx->phddo = phddo;
	dx->UsageCount = 1;
	KeInitializeEvent( &dx->StoppingEvent, NotificationEvent, FALSE);
	dx->Stopping = false;
	dx->GotPortOrMemory = false;
	dx->GotInterrupt = false;
	dx->ConnectedToInterrupt = false;
	dx->SetTimeout = 10;
	dx->Timeout = -1;
	dx->StopTimer = false;
	dx->WriteCmds = NULL;
	dx->ReadCmds = NULL;
	dx->StartReadCmds = NULL;

	// Initialise "connect to interrupt" work queue item
	ExInitializeWorkItem( &dx->ConnectIntWQI, IrqConnectRoutine, dx);
	dx->ConnectIntQueued = false;

	// Initialise timer for this device (but do not start)
	status = IoInitializeTimer( phddo, (PIO_TIMER_ROUTINE)Timeout1s, dx);
	if( !NT_SUCCESS(status))
	{
		DebugPrintMsg("Could not initialise timer");
		IoDeleteDevice(phddo);
		return status;
	}

	// Create a symbolic link so our device is visible to Win32...
	DebugPrint("Creating symbolic link %T",&linkName);
	status = IoCreateSymbolicLink( &linkName, &deviceName);
	if( !NT_SUCCESS(status)) 
	{
		DebugPrintMsg("Could not create symbolic link");
		IoDeleteDevice(phddo);
		return status;
	}
	
	// Initialise our DPC for IRQ completion processing
	IoInitializeDpcRequest( phddo, PHDIoDpcForIsr);

	return status;
}

#pragma code_seg() // end INIT section

//////////////////////////////////////////////////////////////////////////////
//	PHDIoUnload
//
//	Description:
//		Unload the driver by removing any remaining objects, etc.
//
//	Arguments:
//		Pointer to the Driver object
//
//	Return Value:
//		None

#pragma code_seg("PAGE") // start PAGE section

VOID PHDIoUnload(IN PDRIVER_OBJECT DriverObject)
{
	DebugPrintMsg("PHDIoUnload");
	PHDIoDeleteDevice();
	DebugPrintClose();
}

//////////////////////////////////////////////////////////////////////////////

void PHDIoDeleteDevice()
{
	if( phddo==NULL) return;

	PPHDIO_DEVICE_EXTENSION dx = (PPHDIO_DEVICE_EXTENSION)phddo->DeviceExtension;
	StopDevice(dx);

	// Stop timer (not sure if this is necessary)
	if( dx->StopTimer)
		IoStopTimer(phddo);

	// Remove any stored command buffers
	FreeIfAllocated(dx->WriteCmds);
	FreeIfAllocated(dx->StartReadCmds);
	FreeIfAllocated(dx->ReadCmds);

	// Initialise Symbolic link name
	UNICODE_STRING linkName;
	RtlInitUnicodeString( &linkName, SYM_LINK_NAME);

	// Remove symbolic link
	DebugPrint("Deleting symbolic link %T",&linkName);
	IoDeleteSymbolicLink( &linkName);

	// Delete device
	DebugPrintMsg("Deleting phddo device object");
	IoDeleteDevice(phddo);
}

//////////////////////////////////////////////////////////////////////////////
#pragma code_seg() // end PAGE section

/////////////////////////////////////////////////////////////////////////////
//	LockDevice:	Lock out PnP remove request

bool LockDevice( IN PPHDIO_DEVICE_EXTENSION dx)
{
//	DebugPrintMsg("LockDevice");
	InterlockedIncrement(&dx->UsageCount);

	if( dx->Stopping)
	{
		if( InterlockedDecrement(&dx->UsageCount)==0)
			KeSetEvent( &dx->StoppingEvent, 0, FALSE);
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//	UnlockDevice:	Unlock device allow PnP remove request

void UnlockDevice( IN PPHDIO_DEVICE_EXTENSION dx)
{
//	DebugPrintMsg("UnlockDevice");
	LONG UsageCount = InterlockedDecrement(&dx->UsageCount);
	if( UsageCount==0)
	{
		DebugPrintMsg("UnlockDevice: setting StoppingEvent flag");
		KeSetEvent( &dx->StoppingEvent, 0, FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
