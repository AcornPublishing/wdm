//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Wdm2 example
/////////////////////////////////////////////////////////////////////////////
//	wdm2.h			Common header
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
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
//	Spin lock to protect access to shared memory buffer

extern KSPIN_LOCK BufferLock;
extern PUCHAR Buffer;

/////////////////////////////////////////////////////////////////////////////
//	Our device extension

typedef struct _WDM2_DEVICE_EXTENSION
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

	DEVICE_POWER_STATE PowerState;	// Our device power state
	PULONG PowerIdleCounter;		// Device idle counter

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

} WDM2_DEVICE_EXTENSION, *PWDM2_DEVICE_EXTENSION;

/////////////////////////////////////////////////////////////////////////////
// Forward declarations of global functions

VOID Wdm2Unload(IN PDRIVER_OBJECT DriverObject);

NTSTATUS Wdm2Power(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS Wdm2Pnp(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS Wdm2AddDevice(	IN PDRIVER_OBJECT DriverObject,
						IN PDEVICE_OBJECT pdo);

NTSTATUS Wdm2Create(IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS Wdm2Close(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);
 
NTSTATUS Wdm2Write(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS Wdm2Read(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS Wdm2DeviceControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp);

NTSTATUS Wdm2SystemControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp);

/////////////////////////////////////////////////////////////////////////////

bool LockDevice( IN PWDM2_DEVICE_EXTENSION dx);
void UnlockDevice( IN PWDM2_DEVICE_EXTENSION dx);

NTSTATUS SendDeviceSetPower( IN PWDM2_DEVICE_EXTENSION dx, IN DEVICE_POWER_STATE DevicePowerState);

NTSTATUS StartDevice(	IN PWDM2_DEVICE_EXTENSION dx,
						IN PCM_RESOURCE_LIST AllocatedResourcesTranslated);

VOID StopDevice(	IN PWDM2_DEVICE_EXTENSION dx);
void SetPowerState( IN PWDM2_DEVICE_EXTENSION dx, IN DEVICE_POWER_STATE NewDevicePowerState);

/////////////////////////////////////////////////////////////////////////////

NTSTATUS CompleteIrp( IN PIRP Irp, IN NTSTATUS status, IN ULONG info);

/////////////////////////////////////////////////////////////////////////////
