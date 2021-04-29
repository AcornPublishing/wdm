//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998,1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Wdm3 example
/////////////////////////////////////////////////////////////////////////////
//	wdm3.h			Common header
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	10-May-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//	Include WDM standard header with C linkage

#ifdef __cplusplus
extern "C"
{
#endif
#include "wdm.h"
#include "wmilib.h"
#include "wmistr.h"
#include "wdmguid.h"
#ifdef __cplusplus
}
#endif

#include "eventlog.h"
#include "Wdm3Msg.h"

/////////////////////////////////////////////////////////////////////////////
//	DebugPrint and Guid headers

#include "DebugPrint.h"

#include "..\..\GUIDs.h"

/////////////////////////////////////////////////////////////////////////////
//	Spin lock to protect access to shared memory buffer

extern KSPIN_LOCK BufferLock;
extern PUCHAR Buffer;
extern ULONG BufferSize;

/////////////////////////////////////////////////////////////////////////////
//	RegistryPath saved for WMI

extern UNICODE_STRING Wdm3RegistryPath;

/////////////////////////////////////////////////////////////////////////////
//	Our device extension

typedef struct _WDM3_DEVICE_EXTENSION
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

	WMILIB_CONTEXT WmiLibInfo;		// WMI Context
	BOOLEAN IdlePowerDownEnable;	// Enable power down option
	BOOLEAN WMIEventEnabled;		// Enable WMI events

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

} WDM3_DEVICE_EXTENSION, *PWDM3_DEVICE_EXTENSION;

/////////////////////////////////////////////////////////////////////////////
// Forward declarations of global functions

VOID Wdm3Unload(IN PDRIVER_OBJECT DriverObject);

NTSTATUS Wdm3Power(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS Wdm3Pnp(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS Wdm3AddDevice(	IN PDRIVER_OBJECT DriverObject,
						IN PDEVICE_OBJECT pdo);

NTSTATUS Wdm3Create(IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS Wdm3Close(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);
 
NTSTATUS Wdm3Write(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS Wdm3Read(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS Wdm3DeviceControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp);

NTSTATUS Wdm3SystemControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp);

/////////////////////////////////////////////////////////////////////////////

bool LockDevice( IN PWDM3_DEVICE_EXTENSION dx);
void UnlockDevice( IN PWDM3_DEVICE_EXTENSION dx);

NTSTATUS SendDeviceSetPower( IN PWDM3_DEVICE_EXTENSION dx, IN DEVICE_POWER_STATE DevicePowerState);

NTSTATUS StartDevice(	IN PWDM3_DEVICE_EXTENSION dx,
						IN PCM_RESOURCE_LIST AllocatedResourcesTranslated);

VOID StopDevice(	IN PWDM3_DEVICE_EXTENSION dx);
void SetPowerState( IN PWDM3_DEVICE_EXTENSION dx, IN DEVICE_POWER_STATE NewDevicePowerState);

void RegisterWmi( IN PDEVICE_OBJECT fdo);
void DeregisterWmi( IN PDEVICE_OBJECT fdo);
void Wdm3FireEvent( IN PDEVICE_OBJECT fdo, wchar_t* Msg);

/////////////////////////////////////////////////////////////////////////////

NTSTATUS CompleteIrp( IN PIRP Irp, IN NTSTATUS status, IN ULONG info);

/////////////////////////////////////////////////////////////////////////////
