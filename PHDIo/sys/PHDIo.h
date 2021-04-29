//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	PHDIo example
/////////////////////////////////////////////////////////////////////////////
//	PHDIo.h			Common header
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
//	11-May-99	1.0.3	CC	RunCmdsOutBuffer added
//	14-May-99	1.0.3	CC	ConnectIntWQI, ConnectIntQueued and IrqConnectRoutine added
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//	Include NT DDK standard header with C linkage

#ifdef __cplusplus
extern "C"
{
#endif
#include "ntddk.h"
#ifdef __cplusplus
}
#endif

/////////////////////////////////////////////////////////////////////////////
//	DebugPrint and Guid headers

#include "DebugPrint.h"

//	#include "..\..\GUIDs.h"

/////////////////////////////////////////////////////////////////////////////
//	Our device extension

typedef struct _PHDIO_DEVICE_EXTENSION
{
	PDEVICE_OBJECT	phddo;

	LONG UsageCount;		// Pending I/O Count
	bool Stopping;			// In process of stopping
	KEVENT StoppingEvent;	// Set when all pending I/O complete


	// Resource allocations
	BOOLEAN ResourceOverride;

	bool GotPortOrMemory;
	bool PortInIOSpace;
	bool PortNeedsMapping;
	PUCHAR PortBase;
	PHYSICAL_ADDRESS PortStartAddress;
	ULONG PortLength;

	bool GotInterrupt;
	ULONG Vector;
	KIRQL Irql;
	KINTERRUPT_MODE Mode;
	KAFFINITY Affinity;
	PKINTERRUPT InterruptObject;

	// Interrupt handling support
	bool ConnectedToInterrupt;
	UCHAR InterruptReg;
	UCHAR InterruptRegMask;
	UCHAR InterruptRegValue;
	WORK_QUEUE_ITEM ConnectIntWQI;	// Work queue item for connecting to interrupt 
	bool ConnectIntQueued;			// Work queue item queued: StartIo doesn't complete IRP

	ULONG TxTotal;		// R/W total transfer size in bytes
	ULONG TxLeft;		// R/W bytes left to transfer
	PUCHAR TxBuffer;	// R/W buffer.  Moves through current IRP SystemBuffer
	bool TxIsWrite;		// R/W direction
	NTSTATUS TxStatus;	// R/W status return
	UCHAR TxResult[5];	// R/W output buffer (2 Failcode, 2 Offset, 1 user)
	UCHAR TxLastIntReg;	// R/W last interrupt register value
	ULONG TxCmdOutputCount;	// R/W Copy of last CmdOutputCount

	PUSHORT RunCmdsOutBuffer;// Output buffer during RUN_CMDS
	ULONG CmdOutputCount;	// Count of bytes output from commands

	UCHAR SetTimeout;	// Timeout stored from script
	int Timeout;		// Seconds left to go.  -1 if not in force
	bool StopTimer;		// Set to stop timer

	PUCHAR WriteCmds;	// Stored commands for write IRP
	ULONG WriteCmdsLen;	//							 length
	PUCHAR StartReadCmds;	// Stored commands for start read IRP
	ULONG StartReadCmdsLen;	//						 		  length
	PUCHAR ReadCmds;	// Stored commands for read IRP
	ULONG ReadCmdsLen;	//							length

} PHDIO_DEVICE_EXTENSION, *PPHDIO_DEVICE_EXTENSION;

/////////////////////////////////////////////////////////////////////////////
// Forward declarations of global functions

VOID PHDIoUnload(IN PDRIVER_OBJECT DriverObject);

NTSTATUS PHDIoCreate(IN PDEVICE_OBJECT phddo,
					 IN PIRP Irp);

NTSTATUS PHDIoClose(	IN PDEVICE_OBJECT phddo,
						IN PIRP Irp);
 
NTSTATUS PHDIoWrite(	IN PDEVICE_OBJECT phddo,
						IN PIRP Irp);

NTSTATUS PHDIoRead(	IN PDEVICE_OBJECT phddo,
					IN PIRP Irp);

NTSTATUS PHDIoDeviceControl(	IN PDEVICE_OBJECT phddo,
								IN PIRP Irp);

VOID PHDIoStartIo(	IN PDEVICE_OBJECT phddo,
					IN PIRP Irp);

VOID PHDIoDpcForIsr(IN PKDPC Dpc, IN PDEVICE_OBJECT phddo, 
					IN PIRP Irp, IN PPHDIO_DEVICE_EXTENSION dx);

VOID Timeout1s( IN PDEVICE_OBJECT phddo, IN PPHDIO_DEVICE_EXTENSION dx);

VOID PHDIoCancelIrp( IN PDEVICE_OBJECT phddo, IN PIRP Irp);

NTSTATUS PHDIoDispatchCleanup( IN PDEVICE_OBJECT phddo, IN PIRP Irp);

VOID IrqConnectRoutine( IN PVOID Context);

/////////////////////////////////////////////////////////////////////////////

bool LockDevice( IN PPHDIO_DEVICE_EXTENSION dx);
void UnlockDevice( IN PPHDIO_DEVICE_EXTENSION dx);

VOID StopDevice(	IN PPHDIO_DEVICE_EXTENSION dx);

#define FreeIfAllocated(x) if( (x)!=NULL) { ExFreePool(x); (x) = NULL; }

/////////////////////////////////////////////////////////////////////////////

NTSTATUS CompleteIrp( IN PIRP Irp, IN NTSTATUS status, IN ULONG info=0, IN CCHAR PriorityBoost=IO_NO_INCREMENT);

/////////////////////////////////////////////////////////////////////////////
