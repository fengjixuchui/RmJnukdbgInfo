copy "%~dp0RmJnukdbgInfo.sys" "C:\RmJnukdbgInfo.sys"
sc create RmJnukdbgInfo binPath= "\??\c:\RmJnukdbgInfo.sys" type= "kernel" start= "system"
sc start RmJnukdbgInfo
