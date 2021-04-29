prebuild:	Wdm3Msg.h Wdm3.bmf

Wdm3.bmf:	Wdm3.mof
	mofcomp -B:Wdm3.bmf -WMI Wdm3.mof

Wdm3Msg.rc Wdm3Msg.h: Wdm3Msg.mc
	mc -v -c Wdm3Msg.mc

PostBuildSteps: $(TARGET)
!if "$(DDKBUILDENV)"=="free"
	rebase -B 0x10000 -X . $(TARGET)
!endif
	copy $(TARGET) $(WINDIR)\system32\drivers
