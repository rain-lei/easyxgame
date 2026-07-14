Option Explicit

' Convert a generated DOCX report to the genuine Word 97-2003 binary format.
' Saving through Word preserves fields, pagination and the legacy compound-file structure.

Dim inputPath, outputPath, pdfPath, sourcePdfPath, word, document, toc
If WScript.Arguments.Count < 2 Or WScript.Arguments.Count > 4 Then
    WScript.Echo "usage: word_save_as_doc.vbs input.docx output.doc [doc-preview.pdf] [docx-preview.pdf]"
    WScript.Quit 2
End If

inputPath = WScript.Arguments(0)
outputPath = WScript.Arguments(1)
pdfPath = ""
sourcePdfPath = ""
If WScript.Arguments.Count = 3 Then
    pdfPath = WScript.Arguments(2)
End If
If WScript.Arguments.Count >= 4 Then
    pdfPath = WScript.Arguments(2)
    sourcePdfPath = WScript.Arguments(3)
End If

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

' The optional source preview captures the updated DOCX before compatibility save.
' Comparing it with the post-save preview detects reflow or image changes.
If sourcePdfPath <> "" Then
    Err.Clear
    document.ExportAsFixedFormat sourcePdfPath, 17
    If Err.Number <> 0 Then
        WScript.Echo "Source ExportAsFixedFormat failed: " & Err.Description
        document.Close False
        word.Quit
        WScript.Quit 7
    End If
End If

' 0 = wdFormatDocument, the real Word 97-2003 binary .doc format.
Err.Clear
document.SaveAs2 outputPath, 0
If Err.Number <> 0 Then
    WScript.Echo "SaveAs2 failed: " & Err.Description
    document.Close False
    word.Quit
    WScript.Quit 5
End If

' Export from the same in-memory document so the preview reflects the legacy save
' without reopening the binary file in a second compatibility-mode session.
If pdfPath <> "" Then
    Err.Clear
    document.ExportAsFixedFormat pdfPath, 17
    If Err.Number <> 0 Then
        WScript.Echo "ExportAsFixedFormat failed: " & Err.Description
        document.Close False
        word.Quit
        WScript.Quit 6
    End If
End If

document.Close False
word.Quit
WScript.Echo "WROTE " & outputPath
If pdfPath <> "" Then
    WScript.Echo "WROTE " & pdfPath
End If
If sourcePdfPath <> "" Then
    WScript.Echo "WROTE " & sourcePdfPath
End If
WScript.Quit 0
