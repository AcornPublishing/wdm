//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	WdmIo example
/////////////////////////////////////////////////////////////////////////////
//	WdmIo.h			Common header
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	14-May-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//	Include WDM standard header with C linkage

#ifdef __cplusplus
extern "C"
{
#endif
#include "wdm.h"
#ifdef __cplusplus
}
#endif

/////////////////////////////////////////////////////////////////////////////
//	DebugPrint and Guid headers

#include "DebugPrint.h"

#include "..\..\GUIDs.h"

/////////////////////////////////////////////////////////////////////////////
//	Our device extension

typedef struct _WDMIO_DEVICE_EXTENSION
{
	PDEVICE_OBJECT	fdo;
	PDEVICE_OBJECT	pdo;
	PDEVICE_OBJECT	NextStackDevice;
	UNICODE_STRING	ifSymLinkName;

	bool GotResources;	// Not stopped
	bool Paused;		// Stop or remove pending
	bool IODisabled;	// Paused or stopped
//	bool InterruptsEnabled;

	LONG OpenHandleCount;	// Count of open handles

	LONG UsageCount;		// Pending I/O Count
	bool Stopping;			// In process of stopping
	KEVENT StoppingEvent;	// Set when all pending I/O complete


	// Resource allocations
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

	PUCHAR RunCmdsOutBuffer;// Output buffer during RUN_CMDS
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

} WDMIO_DEVICE_EXTENSION, *PWDMIO_DEVICE_EXTENSION;

/////////////////////////////////////////////////////////////////////////////
// Forward declarations of global functions

VOID WdmIoUnload(IN PDRIVER_OBJECT DriverObject);

NTSTATUS WdmIoPower(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS WdmIoPnp(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS WdmIoAddDevice(	IN PDRIVER_OBJECT DriverObject,
						IN PDEVICE_OBJECT pdo);

NTSTATUS WdmIoCreate(IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS WdmIoClose(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);
 
NTSTATUS WdmIoWrite(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS WdmIoRead(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS WdmIoDeviceControl(	IN PDEVICE_OBJECT fdo,
								IN PIRP Irp);

NTSTATUS WdmIoSystemControl(	IN PDEVICE_OBJECT fdo,
								IN PIRP Irp);

VOID WdmIoStartIo(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

VOID WdmIoDpcForIsr(IN PKDPC Dpc, IN PDEVICE_OBJECT fdo, 
					IN PIRP Irp, IN PWDMIO_DEVICE_EXTENSION dx);

VOID Timeout1s( IN PDEVICE_OBJECT fdo, IN PWDMIO_DEVICE_EXTENSION dx);

VOID WdmIoCancelIrp( IN PDEVICE_OBJECT fdo, IN PIRP Irp);

NTSTATUS WdmIoDispatchCleanup( IN PDEVICE_OBJECT fdo, IN PIRP Irp);

VOID IrqConnectRoutine( IN PVOID Context);

/////////////////////////////////////////////////////////////////////////////

bool LockDevice( IN PWDMIO_DEVICE_EXTENSION dx);
void UnlockDevice( IN PWDMIO_DEVICE_EXTENSION dx);

NTSTATUS StartDevice(	IN PWDMIO_DEVICE_EXTENSION dx,
						IN PCM_RESOURCE_LIST AllocatedResourcesTranslated);

VOID StopDevice(	IN PWDMIO_DEVICE_EXTENSION dx);

#define FreeIfAllocated(x) if( (x)!=NULL) { ExFreePool(x); (x) = NULL; }

/////////////////////////////////////////////////////////////////////////////

NTSTATUS CompleteIrp( IN PIRP Irp, IN NTSTATUS status, IN ULONG info=0, IN CCHAR PriorityBoost=IO_NO_INCREMENT);

/////////////////////////////////////////////////////////////////////////////
