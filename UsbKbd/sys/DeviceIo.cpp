//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	UsbKbd example
/////////////////////////////////////////////////////////////////////////////
//	DeviceIo.cpp:	Routines that interact with a device
//					Real I/O has been commented out
/////////////////////////////////////////////////////////////////////////////
//	StartDevice			Start the device
//*	RetrieveResources	Get resources from given list
//	StopDevice			Stop device
//	SetPowerState		Set power state
//*	DisableDeviceInterrupts		Disable device interrupts
//*	EnableDeviceInterrupts		Enable device interrupts
//*	InterruptHandler	Handle interrupts
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "UsbKbd.h"

#pragma code_seg("PAGE")	// start PAGE section

/////////////////////////////////////////////////////////////////////////////

NTSTATUS RetrieveResources(IN PUSBKBD_DEVICE_EXTENSION dx,IN PCM_RESOURCE_LIST AllocatedResourcesTranslated);
//bool DisableDeviceInterrupts( IN PUSBKBD_DEVICE_EXTENSION dx);
//bool EnableDeviceInterrupts( IN PUSBKBD_DEVICE_EXTENSION dx);
//BOOLEAN InterruptHandler(IN PKINTERRUPT Interrupt, IN PUSBKBD_DEVICE_EXTENSION fdo);

/////////////////////////////////////////////////////////////////////////////
//	StartDevice:	Start the device

NTSTATUS StartDevice( IN PUSBKBD_DEVICE_EXTENSION dx, IN PCM_RESOURCE_LIST AllocatedResourcesTranslated)
{
	if( dx->GotResources)
		return STATUS_SUCCESS;

	NTSTATUS status = RetrieveResources(dx,AllocatedResourcesTranslated);
	if( !NT_SUCCESS(status))
		return status;
/*
	// Map memory
	if( dx->PortNeedsMapping)
	{
		dx->PortBase = (PUCHAR)MmMapIoSpace( dx->PortStartAddress, dx->PortLength, MmNonCached);
		if( !dx->PortBase)
			return STATUS_NO_MEMORY;
	}
	else
		dx->PortBase = (PUCHAR)dx->PortStartAddress.LowPart;

	// Disable device
	if( !KeSynchronizeExecution( dx->InterruptObject, (PKSYNCHRONIZE_ROUTINE)DisableDeviceInterrupts, (PVOID)dx))
	{
		if( dx->PortNeedsMapping)
			MmUnmapIoSpace( dx->PortBase, dx->PortLength);
		return STATUS_INVALID_DEVICE_STATE;
	}

	// Connect to interrupts
	if( dx->GotInterrupt)
	{
		status = IoConnectInterrupt( &dx->InterruptObject, (PKSERVICE_ROUTINE)InterruptHandler,
							(PVOID)dx, NULL, dx->Vector, dx->Irql, dx->Irql, dx->Mode, FALSE, dx->Affinity, FALSE);
		if( !NT_SUCCESS(status))
		{
			if( dx->PortNeedsMapping)
				MmUnmapIoSpace( dx->PortBase, dx->PortLength);
			return status;
		}
	}

	// Enable device
	if( !KeSynchronizeExecution( dx->InterruptObject, (PKSYNCHRONIZE_ROUTINE)EnableDeviceInterrupts, (PVOID)dx))
	{
		if( dx->GotInterrupt)
			IoDisconnectInterrupt(dx->InterruptObject);
		if( dx->PortNeedsMapping)
			MmUnmapIoSpace( dx->PortBase, dx->PortLength);
		return STATUS_INVALID_DEVICE_STATE;
	}
*/

	// Device is now started
	dx->GotResources = true;
	
	return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
//	RetrieveResources:	Get resources from given list

NTSTATUS RetrieveResources( IN PUSBKBD_DEVICE_EXTENSION dx, IN PCM_RESOURCE_LIST AllocatedResourcesTranslated)
{
	if( AllocatedResourcesTranslated==NULL ||
		AllocatedResourcesTranslated->Count==0)
	{
		DebugPrintMsg("RetrieveResources: No allocated translated resources");
		return STATUS_SUCCESS;	// or whatever
	}

	// Get to actual resources
	PCM_PARTIAL_RESOURCE_LIST list = &AllocatedResourcesTranslated->List[0].PartialResourceList;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR resource = list->PartialDescriptors;
	ULONG NumResources = list->Count;

	DebugPrint("RetrieveResources: %d resource lists %d resources", AllocatedResourcesTranslated->Count, NumResources);

	bool GotError = false;

	// Clear dx
	dx->GotInterrupt = false;
	dx->GotPortOrMemory = false;
	dx->PortInIOSpace = false;
	dx->PortNeedsMapping = false;

	// Go through each allocated resource
	for( ULONG i=0; i<NumResources; i++,resource++)
	{
		switch( resource->Type)
		{
		case CmResourceTypePort:
			if( dx->GotPortOrMemory) { GotError = true; break; }
			dx->GotPortOrMemory = true;
			dx->PortStartAddress = resource->u.Port.Start;
			dx->PortLength = resource->u.Port.Length;
			dx->PortNeedsMapping = (resource->Flags & CM_RESOURCE_PORT_IO)==0;
			dx->PortInIOSpace = !dx->PortNeedsMapping;
			DebugPrint("RetrieveResources: Port %L Length %d NeedsMapping %d",
							dx->PortStartAddress,
							dx->PortLength, dx->PortNeedsMapping);
			break;

		case CmResourceTypeInterrupt:
			dx->GotInterrupt = true;
			dx->Irql = (KIRQL)resource->u.Interrupt.Level;
			dx->Vector = resource->u.Interrupt.Vector;
			dx->Affinity = resource->u.Interrupt.Affinity;
			dx->Mode = (resource->Flags == CM_RESOURCE_INTERRUPT_LATCHED)
						? Latched : LevelSensitive;
			DebugPrint("RetrieveResources: Interrupt vector %x IRQL %d Affinity %d Mode %d",
							dx->Vector, dx->Irql, dx->Affinity, dx->Mode);
			break;

		case CmResourceTypeMemory:
			if( dx->GotPortOrMemory) { GotError = true; break; }
			dx->GotPortOrMemory = true;
			dx->PortStartAddress = resource->u.Memory.Start;
			dx->PortLength = resource->u.Memory.Length;
			dx->PortNeedsMapping = true;
			DebugPrint("RetrieveResources: Memory %L Length %d",
							dx->PortStartAddress, dx->PortLength);
			break;

		case CmResourceTypeDma:
		case CmResourceTypeDeviceSpecific:
		case CmResourceTypeBusNumber:
		default:
			DebugPrint("RetrieveResources: Unrecognised resource type %d", resource->Type);
			GotError = true;
			break;
		}
	}

	// Check we've got the resources we need
	if( GotError /*|| !GotPortOrMemory || !GotInterrupt*/)
		return STATUS_DEVICE_CONFIGURATION_ERROR;

	return STATUS_SUCCESS;
}
	
/////////////////////////////////////////////////////////////////////////////
//	StopDevice:	Stop device

VOID StopDevice( IN PUSBKBD_DEVICE_EXTENSION dx)
{
	DebugPrintMsg("StopDevice");
	if( !dx->GotResources)
		return;
	dx->GotResources = false;

/*
	// Disable device
	KeSynchronizeExecution( dx->InterruptObject, (PKSYNCHRONIZE_ROUTINE)DisableDeviceInterrupts, (PVOID)dx); 
	// Disconnect from interrupt
	if( dx->GotInterrupt)
		IoDisconnectInterrupt( dx->InterruptObject);
	dx->InterruptObject = NULL;
	// Unmap memory
	if (dx->PortNeedsMapping)
		MmUnmapIoSpace( (PVOID)dx->PortBase, dx->PortLength);
*/
}

/////////////////////////////////////////////////////////////////////////////
/*
void WriteByte( IN PUSBKBD_DEVICE_EXTENSION dx, IN UCHAR byte, IN ULONG offset)
{
	if (dx->PortInIOSpace)
		WRITE_PORT_UCHAR(dx->PortBase+offset, byte);
	else
		WRITE_REGISTER_UCHAR(dx->PortBase+offset, byte);
}
*/
/////////////////////////////////////////////////////////////////////////////

#pragma code_seg()	// end PAGE section

/////////////////////////////////////////////////////////////////////////////
//	DisableDeviceInterrupts:	Disable device interrupts
/*
bool DisableDeviceInterrupts( IN PUSBKBD_DEVICE_EXTENSION dx)
{
	DebugPrintMsg("DisableDeviceInterrupts");
	WriteByte( CONTROL_REGISTER, ~INTERRUPTS_ENABLED);
	dx->Enabled = false;
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//	EnableDeviceInterrupts:	Enable device interrupts

bool EnableDeviceInterrupts( IN PUSBKBD_DEVICE_EXTENSION dx)
{
	DebugPrintMsg("EnableDeviceInterrupts");
	WriteByte( CONTROL_REGISTER, INTERRUPTS_ENABLED);
	dx->Enabled = true;
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//	InterruptHandler:	Handle interrupts

BOOLEAN InterruptHandler(IN PKINTERRUPT Interrupt, IN PUSBKBD_DEVICE_EXTENSION dx)
{
	// Do not call DebugPrint here
	return FALSE;
}
*/
/////////////////////////////////////////////////////////////////////////////

