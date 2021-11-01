@if "%1" == "ovmf" (
  goto ovmf
)

call py UefiPayloadPkg\UniversalPayloadBuild.py -t CLANGPDB 
if not %ERRORLEVEL% == 0 exit /b 1


:ovmf
call build -p OvmfPkg\QemuUniversalPayload\OvmfPkgPol.dsc -a IA32 -a X64 -D DEBUG_ON_SERIAL_PORT -D UNIVERSAL_PAYLOAD=TRUE -t CLANGPDB -y ovmflog.txt

if not %ERRORLEVEL% == 0 exit /b 1
call C:\code\Payload\edk2_nanopb2\qemu\qemu-system-x86_64.exe -machine q35 -drive file=C:\code\Payload\edk2-for_upstream_test\Build\OvmfPol\DEBUG_CLANGPDB\FV\OVMF.fd,if=pflash,format=raw -boot menu=on,splash-time=0 -usb -device nec-usb-xhci,id=xhci -device usb-kbd   -device usb-mouse -net none -serial mon:stdio > bootlog.txt