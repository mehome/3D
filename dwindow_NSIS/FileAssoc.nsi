!include "logiclib.nsh"
!ifndef Project
!define Project AssocTool   ;�ĳ�����������
!endif

;�����ļ����÷���!insertmacro Assoc dts "dts" "DVD �ļ�" "MCP.exe" "MCP.ico"
!macro Assoc EXT TYPE DESC OPENEXE ICO
Push $0
Push $1
Push $2
Push $3

ReadRegStr $1 HKCR ".${EXT}" ""                 ;�����ڵ�

${if} $1 == ""
  DetailPrint '$(Associatedfiletypes) *.${ext} $(For) "${desc}"...'
  WriteRegStr HKCR "Back.${Project}\.${ext}" "" "_Blank_"
${Else}
  DetailPrint '$(Modifyfiletypes) *.${ext} $(For) "${desc}"...'
  StrLen $3 ${Project}
  StrCpy $2 $1 $3
  ${if} $2 != "${Project}"
  	WriteRegStr HKCR "Back.${Project}\.${ext}" "" "$1"   ;����������ǹ����ģ����ݸ���չ��
  ${Endif}
${EndIf}
;���ڻ���������
WriteRegStr HKCR ".${ext}" "" "${Project}.${type}"
WriteRegStr HKCR "${Project}.${type}" "" "${desc}"
WriteRegStr HKCR "${Project}.${type}\shell" "" "open"
WriteRegStr HKCR "${Project}.${type}\shell\open" "" "$(PLAY)"
WriteRegStr HKCR "${Project}.${type}\shell\open\command" "" '${openexe} "%1"'
WriteRegStr HKCR "${Project}.${type}\DefaultIcon" "" "${ico}"
;����й����ļ�(ֻҪ�ǿռ���)
WriteRegStr HKCR "Back.${Project}" "" "${Project} Backup"
Pop $3
Pop $2
Pop $1
!macroend

;ȡ���������÷���!insertmacro UnAssoc dts
!macro UnAssoc EXT

Push $1
Push $2  ;Content Type
Push $3  ;CLSID
;**********�޸����´���Ҫ����!**********

ReadRegStr $1 HKCR "Back.${Project}\.${EXT}" ""     ;������
ReadRegStr $2 HKCR ".${EXT}" ""                 ;�����ڵ�

StrLen $3 ${Project}
StrCpy $4 $2 $3

${if} "$4" == "${Project}"        ;������ڻ������ǹ�����
${if} "$1" == "_Blank_"  ;���������"_Blank_",֤���Ѿ�����,��û�о�����.
    WriteRegStr HKCR ".${EXT}" "" ""           ;��������
${Else}
    ${if} "$1" != ""
    WriteRegStr HKCR ".${EXT}" "" "$1"         ;�ָ�����
    ${EndIf}
${EndIf}
DeleteRegKey HKCR "Back.${Project}\.${ext}"   ;�ָ����,ɾ������
StrCmp "${Project}.${ext}" "." +2
DeleteRegKey HKCR "${Project}.${ext}"   ;�ָ����,ɾ������
${EndIf}
Pop $3
Pop $2
Pop $1
!macroend