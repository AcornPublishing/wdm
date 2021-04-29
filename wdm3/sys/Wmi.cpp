//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998,1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Wdm3 example
/////////////////////////////////////////////////////////////////////////////
//	wmi.cpp:			WMI System Control IRP handler
/////////////////////////////////////////////////////////////////////////////
//	RegisterWmi			Register our WMI support
//	DeregisterWmi		Deregister our WMI support
//	Wdm3SystemControl	Handle IRP_MJ_SYSTEM_CONTROL requests
//*	FailWMIRequest		Fail an unrecognised WMI request
//*	SetWmiDataItem		Call-back to set an item in a data block
//*	SetWmiDataBlock		Call-back to store properties for a data block
//*	QueryWmiDataBlock	Call-back to return the properties of one or more data blocks
//*	QueryWmiRegInfo		Call-back to provide further WMI info about the blocks we are using
//*	WmiFunctionControl	Call-back to enable events or expensive block collection
//	Wdm3FireEvent		Send off a Wdm3Event event enclosing given message
//*	ExecuteWmiMethod	Call-back to execute the specified method
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	19-May-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////


#include "Wdm3.h"

#pragma code_seg("PAGE")	// start PAGE section

//////////////////////////////////////////////////////////////////////////////

UNICODE_STRING Wdm3RegistryPath;

//////////////////////////////////////////////////////////////////////////////

NTSTATUS SetWmiDataItem(	IN PDEVICE_OBJECT fdo, IN PIRP Irp,
							IN ULONG GuidIndex, IN ULONG InstanceIndex,
							IN ULONG DataItemId,
							IN ULONG BufferSize, IN PUCHAR PBuffer);

NTSTATUS SetWmiDataBlock(	IN PDEVICE_OBJECT fdo, IN PIRP Irp,
							IN ULONG GuidIndex, IN ULONG InstanceIndex,
							IN ULONG BufferSize,
							IN PUCHAR PBuffer);

NTSTATUS QueryWmiDataBlock(	IN PDEVICE_OBJECT fdo, IN PIRP Irp,
							IN ULONG GuidIndex, 
							IN ULONG InstanceIndex,
							IN ULONG InstanceCount,
							IN OUT PULONG InstanceLengthArray,
							IN ULONG OutBufferSize,
							OUT PUCHAR PBuffer);

NTSTATUS QueryWmiRegInfo(	IN PDEVICE_OBJECT fdo, OUT PULONG PRegFlags,
							OUT PUNICODE_STRING PInstanceName,
							OUT PUNICODE_STRING *PRegistryPath,
							OUT PUNICODE_STRING MofResourceName,
							OUT PDEVICE_OBJECT *Pdo);

NTSTATUS WmiFunctionControl(IN PDEVICE_OBJECT fdo,
							IN PIRP Irp,
							IN ULONG GuidIndex,
							IN WMIENABLEDISABLECONTROL Function,
							IN BOOLEAN Enable
							);

NTSTATUS ExecuteWmiMethod(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp,
							IN ULONG GuidIndex,
							IN ULONG InstanceIndex,
							IN ULONG MethodId,
							IN ULONG InBufferSize,
							IN ULONG OutBufferSize,
							IN OUT PUCHAR Buffer
							);

//////////////////////////////////////////////////////////////////////////////
//	List of GUIDs

const int GUID_COUNT = 3;

WMIGUIDREGINFO Wdm3GuidList[GUID_COUNT];// =	//	Do not initialise here:
//{													causes some obscure unload error
//	{ &WDM3_WMI_GUID, 1, 0 },			// Wdm3Information
//	{ &GUID_POWER_DEVICE_ENABLE, 1, 0},	// MSPower_DeviceEnable
//	{ &WDM3_WMI_EVENT_GUID, 1, 0},		// Wdm3Event
//};

const ULONG WDM3_WMI_GUID_INDEX = 0;
const ULONG GUID_POWER_DEVICE_ENABLE_INDEX = 1;
const ULONG WDM3_WMI_EVENT_GUID_INDEX = 2;

//////////////////////////////////////////////////////////////////////////////
//	RegisterWmi:	Register our WMI support

void RegisterWmi( IN PDEVICE_OBJECT fdo)
{
	Wdm3GuidList[WDM3_WMI_GUID_INDEX].Guid = &WDM3_WMI_GUID;
	Wdm3GuidList[WDM3_WMI_GUID_INDEX].InstanceCount = 1;
	Wdm3GuidList[WDM3_WMI_GUID_INDEX].Flags = 0;
	Wdm3GuidList[GUID_POWER_DEVICE_ENABLE_INDEX].Guid = &GUID_POWER_DEVICE_ENABLE;
	Wdm3GuidList[GUID_POWER_DEVICE_ENABLE_INDEX].InstanceCount = 1;
	Wdm3GuidList[GUID_POWER_DEVICE_ENABLE_INDEX].Flags = 0;
	Wdm3GuidList[WDM3_WMI_EVENT_GUID_INDEX].Guid = &WDM3_WMI_EVENT_GUID;
	Wdm3GuidList[WDM3_WMI_EVENT_GUID_INDEX].InstanceCount = 1;
	Wdm3GuidList[WDM3_WMI_EVENT_GUID_INDEX].Flags = 0;

	PWDM3_DEVICE_EXTENSION dx=(PWDM3_DEVICE_EXTENSION)fdo->DeviceExtension;

	dx->WmiLibInfo.GuidCount = GUID_COUNT;
	dx->WmiLibInfo.GuidList = Wdm3GuidList;

	dx->WmiLibInfo.QueryWmiRegInfo = QueryWmiRegInfo;
	dx->WmiLibInfo.QueryWmiDataBlock = QueryWmiDataBlock;
	dx->WmiLibInfo.SetWmiDataBlock = SetWmiDataBlock;
	dx->WmiLibInfo.SetWmiDataItem = SetWmiDataItem;
	dx->WmiLibInfo.ExecuteWmiMethod = ExecuteWmiMethod;
	dx->WmiLibInfo.WmiFunctionControl = WmiFunctionControl;

  	NTSTATUS status = IoWMIRegistrationControl( fdo, WMIREG_ACTION_REGISTER);
	DebugPrint("RegisterWmi %x",status);
}

//////////////////////////////////////////////////////////////////////////////
//	DeregisterWmi:	Deregister our WMI support

void DeregisterWmi( IN PDEVICE_OBJECT fdo)
{
	IoWMIRegistrationControl( fdo, WMIREG_ACTION_DEREGISTER);
	DebugPrintMsg("DeregisterWmi");
}

/////////////////////////////////////////////////////////////////////////////
//	Wdm3SystemControl:
//
//	Description:
//		Handle IRP_MJ_SYSTEM_CONTROL requests
//
//	Arguments:
//		Pointer to our FDO
//		Pointer to the IRP
//			Various minor parameters
//			IrpStack->Parameters.WMI.xxx has WMI parameters
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS Wdm3SystemControl( IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	PWDM3_DEVICE_EXTENSION dx = (PWDM3_DEVICE_EXTENSION)fdo->DeviceExtension;
	if( dx->IODisabled)
		return CompleteIrp( Irp, STATUS_DEVICE_NOT_CONNECTED, 0);
	if (!LockDevice(dx))
		return CompleteIrp( Irp, STATUS_DELETE_PENDING, 0);

	DebugPrintMsg("Wdm3SystemControl");

	SYSCTL_IRP_DISPOSITION disposition;

	NTSTATUS status = WmiSystemControl( &dx->WmiLibInfo, fdo, Irp, &disposition);
	switch(disposition)
	{
	case IrpProcessed:
		// This irp has been processed and may be completed or pending.
		break;

	case IrpNotCompleted:
		// This irp has not been completed, but has been fully processed.
		// we will complete it now
		IoCompleteRequest( Irp, IO_NO_INCREMENT);                
		break;

	case IrpForward:
	case IrpNotWmi:
		// This irp is either not a WMI irp or is a WMI irp targetted
		// at a device lower in the stack.
		IoSkipCurrentIrpStackLocation(Irp);
		status = IoCallDriver( dx->NextStackDevice, Irp);
		break;

	default:
		DebugPrint("Wdm3SystemControl bad disposition %d",disposition);
		IoCompleteRequest( Irp, IO_NO_INCREMENT);                
//		ASSERT(FALSE);
	}
    
	UnlockDevice(dx);
	return status;
}

//////////////////////////////////////////////////////////////////////////////
//	FailWMIRequest:	Fail an unrecognised WMI request.


NTSTATUS FailWMIRequest(
	IN PDEVICE_OBJECT fdo,
	IN PIRP Irp,
	IN ULONG GuidIndex)
{
	DebugPrint("FailWMIRequest: GuidIndex %d",GuidIndex);
	NTSTATUS status;

	if( GuidIndex>=GUID_COUNT)
		status = STATUS_WMI_GUID_NOT_FOUND;
	else
		status = STATUS_INVALID_DEVICE_REQUEST;

	status = WmiCompleteRequest( fdo, Irp, status, 0, IO_NO_INCREMENT);

	return status;
}

//////////////////////////////////////////////////////////////////////////////
//	SetWmiDataItem:	Call-back to set an item in a data block

NTSTATUS SetWmiDataItem(
	IN PDEVICE_OBJECT fdo, IN PIRP Irp,
	IN ULONG GuidIndex, IN ULONG InstanceIndex,
	IN ULONG DataItemId,
	IN ULONG BufferSize, IN PUCHAR PBuffer)
{
	DebugPrint("SetWmiDataItem: GuidIndex %d, InstanceIndex %d, DataItemId %d, BufferSize %d",
		GuidIndex,InstanceIndex,DataItemId,BufferSize);
	return FailWMIRequest( fdo, Irp, GuidIndex);
}

//////////////////////////////////////////////////////////////////////////////
//	SetWmiDataBlock:	Call-back to store properties for a data block

NTSTATUS SetWmiDataBlock(
	IN PDEVICE_OBJECT fdo, IN PIRP Irp,
	IN ULONG GuidIndex, IN ULONG InstanceIndex,
	IN ULONG BufferSize,
	IN PUCHAR PBuffer)
{
	DebugPrint("SetWmiDataBlock: GuidIndex %d, InstanceIndex %d, BufferSize %d",
		GuidIndex,InstanceIndex,BufferSize);
	PWDM3_DEVICE_EXTENSION dx = (PWDM3_DEVICE_EXTENSION)fdo->DeviceExtension;

	if( GuidIndex==GUID_POWER_DEVICE_ENABLE_INDEX)
	{
		if( BufferSize<sizeof(BOOLEAN))
			return WmiCompleteRequest( fdo, Irp, STATUS_BUFFER_TOO_SMALL, 0, IO_NO_INCREMENT);

		// Get Enable property into IdlePowerDownEnable
		dx->IdlePowerDownEnable = *(BOOLEAN*)PBuffer;

		// Action IdlePowerDownEnable
		if( dx->IdlePowerDownEnable)
		{
			DebugPrintMsg("SetWmiDataBlock: Enabling power down");
			// Enable power down idling
			if( dx->PowerIdleCounter==NULL)
				dx->PowerIdleCounter = PoRegisterDeviceForIdleDetection( dx->pdo, 30, 60, PowerDeviceD3);
		}
		else
		{
			DebugPrintMsg("SetWmiDataBlock: Disabling power down");
			// Disable power down idling
			if( dx->PowerIdleCounter!=NULL)
				dx->PowerIdleCounter = PoRegisterDeviceForIdleDetection( dx->pdo, 0, 0, PowerDeviceD3);
			if( dx->PowerState>PowerDeviceD0)
			{
				DebugPrintMsg("SetWmiDataBlock: Disabling power down: power up");
				SendDeviceSetPower( dx, PowerDeviceD0);
			}
		}

		return WmiCompleteRequest( fdo, Irp, STATUS_SUCCESS, 0, IO_NO_INCREMENT);
	}
	return FailWMIRequest( fdo, Irp, GuidIndex);
}

//////////////////////////////////////////////////////////////////////////////
//	QueryWmiDataBlock:	Call-back to return the properties of one or more data blocks

NTSTATUS QueryWmiDataBlock(
	IN PDEVICE_OBJECT fdo, IN PIRP Irp,
	IN ULONG GuidIndex, 
	IN ULONG InstanceIndex,
	IN ULONG InstanceCount,
	IN OUT PULONG InstanceLengthArray,
	IN ULONG OutBufferSize,
	OUT PUCHAR PBuffer)
{
	DebugPrint("QueryWmiDataBlock: GuidIndex %d, InstanceIndex %d, InstanceCount %d, OutBufferSize %d",
		GuidIndex,InstanceIndex,InstanceCount,OutBufferSize);
	PWDM3_DEVICE_EXTENSION dx = (PWDM3_DEVICE_EXTENSION)fdo->DeviceExtension;
	NTSTATUS status;
	ULONG size = 0;

	switch( GuidIndex)
	{
	case WDM3_WMI_GUID_INDEX:	// Wdm3Information
	{
		ULONG SymLinkNameLen = dx->ifSymLinkName.Length;
		size = sizeof(ULONG)+sizeof(ULONG)+SymLinkNameLen+sizeof(USHORT);

		// Check output buffer size
		if( OutBufferSize<size)
		{
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		// Store uint32 BufferLen
		*(ULONG *)PBuffer = BufferSize;
		PBuffer += sizeof(ULONG);

		// Store uint32 BufferFirstWord
		ULONG FirstWord = 0;
		if( Buffer!=NULL && BufferSize>=4)
			FirstWord = *(ULONG*)Buffer;
		*(ULONG *)PBuffer = FirstWord;
		PBuffer += sizeof(ULONG);

		// Store string SymbolicLinkName as counted Unicode
		*(USHORT *)PBuffer = (USHORT)SymLinkNameLen;
		PBuffer += sizeof(USHORT);
		RtlCopyMemory( PBuffer, dx->ifSymLinkName.Buffer, SymLinkNameLen);

		// Store total size
		*InstanceLengthArray = size;
		status = STATUS_SUCCESS;

		break;
	}
	case GUID_POWER_DEVICE_ENABLE_INDEX:	// MSPower_DeviceEnable
	{
		size = sizeof(BOOLEAN);

		// Check output buffer size
		if( OutBufferSize<size)
		{
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		// Store boolean IdlePowerDownEnable in Enable property
		*(BOOLEAN *)PBuffer = dx->IdlePowerDownEnable;
//		PBuffer += sizeof(BOOLEAN);

		// Store total size
		*InstanceLengthArray = size;
		status = STATUS_SUCCESS;

		break;
	}
	default:
		DebugPrintMsg("QueryWmiDataBlock: Bad GUID index");
		status = STATUS_WMI_GUID_NOT_FOUND;
		break;
	}

	return WmiCompleteRequest( fdo, Irp, status, size, IO_NO_INCREMENT);
}

//////////////////////////////////////////////////////////////////////////////
//	QueryWmiRegInfo:	Call-back to provide further WMI info about the blocks we are using

#define MofResourceNameText L"MofResource"

NTSTATUS QueryWmiRegInfo(
	IN PDEVICE_OBJECT fdo, OUT PULONG PRegFlags,
	OUT PUNICODE_STRING PInstanceName,
	OUT PUNICODE_STRING *PRegistryPath,
	OUT PUNICODE_STRING MofResourceName,
	OUT PDEVICE_OBJECT *Pdo)
{
	DebugPrintMsg("QueryWmiRegInfo");
	PWDM3_DEVICE_EXTENSION dx = (PWDM3_DEVICE_EXTENSION)fdo->DeviceExtension;

	*PRegFlags = WMIREG_FLAG_INSTANCE_PDO;
	*PRegistryPath = &Wdm3RegistryPath;
	RtlInitUnicodeString( MofResourceName, MofResourceNameText);
	*Pdo = dx->pdo;

	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//	WmiFunctionControl:	Call-back to enable events or expensive block collection

NTSTATUS WmiFunctionControl(IN PDEVICE_OBJECT fdo,
							IN PIRP Irp,
							IN ULONG GuidIndex,
							IN WMIENABLEDISABLECONTROL Function,
							IN BOOLEAN Enable
							)
{
	DebugPrint("WmiFunctionControl: GuidIndex %d, Function %d, Enable %d",
		GuidIndex,Function,Enable);
	PWDM3_DEVICE_EXTENSION dx = (PWDM3_DEVICE_EXTENSION)fdo->DeviceExtension;

	if( GuidIndex==WDM3_WMI_EVENT_GUID_INDEX && Function==WmiEventControl)
	{
		DebugPrint("WmiFunctionControl: Event enable %d", Enable);
		dx->WMIEventEnabled = Enable;
		return WmiCompleteRequest( fdo, Irp, STATUS_SUCCESS, 0, IO_NO_INCREMENT);
	}
	return FailWMIRequest( fdo, Irp, GuidIndex);
}

//////////////////////////////////////////////////////////////////////////////
//	Wdm3FireEvent:	Send off a Wdm3Event event enclosing given message

void Wdm3FireEvent( IN PDEVICE_OBJECT fdo, wchar_t* Msg)
{
	DebugPrint("Wdm3FireEvent: Msg %S", Msg);
	PWDM3_DEVICE_EXTENSION dx = (PWDM3_DEVICE_EXTENSION)fdo->DeviceExtension;

	if( !dx->WMIEventEnabled) return;

	// Get MsgLen in bytes
	int MsgLen = 0;
	wchar_t* Msg2 = Msg;
	while( *Msg2++!=0)
		MsgLen += sizeof(wchar_t);
	
	// Allocate event memory
	PUSHORT pData = (PUSHORT)ExAllocatePool( NonPagedPool, MsgLen+2);
	if( pData==NULL) return;
	PUSHORT pData2 = pData;
	*pData2++ = MsgLen;
	RtlCopyMemory( pData2, Msg, MsgLen);
	WmiFireEvent( fdo, (LPGUID)&WDM3_WMI_EVENT_GUID, 0, MsgLen+2, pData);
}

//////////////////////////////////////////////////////////////////////////////
//	ExecuteWmiMethod:	Call-back to execute the specified method

NTSTATUS ExecuteWmiMethod(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp,
							IN ULONG GuidIndex,
							IN ULONG InstanceIndex,
							IN ULONG MethodId,
							IN ULONG InBufferSize,
							IN ULONG OutBufferSize,
							IN OUT PUCHAR Buffer
							)
{
	DebugPrint("ExecuteWmiMethod: GuidIndex %d, InstanceIndex %d, MethodId %d, InBufferSize %d OutBufferSize %d",
		GuidIndex,InstanceIndex,MethodId,InBufferSize,OutBufferSize);
	PWDM3_DEVICE_EXTENSION dx = (PWDM3_DEVICE_EXTENSION)fdo->DeviceExtension;

	if( GuidIndex==WDM3_WMI_GUID_INDEX && MethodId==0)
	{
		DebugPrintMsg("ExecuteWmiMethod: PowerDown method");

		// Power Down
		if( dx->PowerState<PowerDeviceD3)
			SendDeviceSetPower( dx, PowerDeviceD3);
		return WmiCompleteRequest( fdo, Irp, STATUS_SUCCESS, 0, IO_NO_INCREMENT);
	}
	return FailWMIRequest( fdo, Irp, GuidIndex);
}

#pragma code_seg()	// end PAGE section

//////////////////////////////////////////////////////////////////////////////
