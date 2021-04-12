Dim pat, patparts, rxp, inp
pat = WScript.Arguments(0)
patparts = Split(pat,"/")
WScript.Echo "//"+ patparts(1) +"=>"+ patparts(2)
Set rxp = new RegExp
rxp.Global = True
rxp.Multiline = True
rxp.Pattern = patparts(1)
Do While Not WScript.StdIn.AtEndOfStream
  inp = WScript.StdIn.ReadLine()
  WScript.Echo rxp.Replace(inp, patparts(2))
Loop