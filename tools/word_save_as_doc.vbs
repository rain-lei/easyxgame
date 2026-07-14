Option Explicit

' Convert a generated DOCX report to the genuine Word 97-2003 binary format.
' The final .doc name is required by the teacher's original submission rules.

Dim inputPath, outputPath, word, document, toc
If WScript.Arguments.Count <> 2 Then
    WScript.Echo "usage: word_save_as_doc.vbs input.docx output.doc"
    WScript.Quit 2
End If

inputPath = WScript.Arguments(0)
outputPath = WScript.Arguments(1)

On Error Resume Next
Set word = CreateObject("Word.Application")
If Err.Number <> 0 Then
    WScript.Echo "CreateObject failed: " & Err.Description
    WScript.Quit 3
End If

word.Visible = False
word.DisplayAlerts = 0
Set document = word.Documents.Open(inputPath, False, True, False)
If Err.Number <> 0 Then
    WScript.Echo "Documents.Open failed: " & Err.Description
    word.Quit
    WScript.Quit 4
End If

' Refresh the table of contents and other fields before writing the legacy file.
document.Fields.Update
For Each toc In document.TablesOfContents
    toc.Update
Next

' 0 = wdFormatDocument, the real Word 97-2003 binary .doc format.
Err.Clear
document.SaveAs2 outputPath, 0
If Err.Number <> 0 Then
    WScript.Echo "SaveAs2 failed: " & Err.Description
    document.Close False
    word.Quit
    WScript.Quit 5
End If

document.Close False
word.Quit
WScript.Echo "WROTE " & outputPath
WScript.Quit 0
