<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class fmMain
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        Me.lvDevices = New System.Windows.Forms.ListView()
        Me.chAddress = CType(New System.Windows.Forms.ColumnHeader(), System.Windows.Forms.ColumnHeader)
        Me.chName = CType(New System.Windows.Forms.ColumnHeader(), System.Windows.Forms.ColumnHeader)
        Me.chStatus = CType(New System.Windows.Forms.ColumnHeader(), System.Windows.Forms.ColumnHeader)
        Me.btStop = New System.Windows.Forms.Button()
        Me.btStart = New System.Windows.Forms.Button()
        Me.lbLog = New System.Windows.Forms.ListBox()
        Me.btClear = New System.Windows.Forms.Button()
        Me.btRead = New System.Windows.Forms.Button()
        Me.btSend = New System.Windows.Forms.Button()
        Me.edData = New System.Windows.Forms.TextBox()
        Me.laDataToSend = New System.Windows.Forms.Label()
        Me.btDisconnect = New System.Windows.Forms.Button()
        Me.SuspendLayout()
        '
        'lvDevices
        '
        Me.lvDevices.Columns.AddRange(New System.Windows.Forms.ColumnHeader() {Me.chAddress, Me.chName, Me.chStatus})
        Me.lvDevices.FullRowSelect = True
        Me.lvDevices.GridLines = True
        Me.lvDevices.HideSelection = False
        Me.lvDevices.Location = New System.Drawing.Point(12, 39)
        Me.lvDevices.MultiSelect = False
        Me.lvDevices.Name = "lvDevices"
        Me.lvDevices.Size = New System.Drawing.Size(419, 140)
        Me.lvDevices.TabIndex = 13
        Me.lvDevices.UseCompatibleStateImageBehavior = False
        Me.lvDevices.View = System.Windows.Forms.View.Details
        '
        'chAddress
        '
        Me.chAddress.Text = "Address"
        Me.chAddress.Width = 120
        '
        'chName
        '
        Me.chName.Text = "Name"
        Me.chName.Width = 120
        '
        'chStatus
        '
        Me.chStatus.Text = "Status"
        Me.chStatus.Width = 120
        '
        'btStop
        '
        Me.btStop.Enabled = False
        Me.btStop.Location = New System.Drawing.Point(93, 10)
        Me.btStop.Name = "btStop"
        Me.btStop.Size = New System.Drawing.Size(75, 23)
        Me.btStop.TabIndex = 12
        Me.btStop.Text = "Stop"
        Me.btStop.UseVisualStyleBackColor = True
        '
        'btStart
        '
        Me.btStart.Location = New System.Drawing.Point(12, 10)
        Me.btStart.Name = "btStart"
        Me.btStart.Size = New System.Drawing.Size(75, 23)
        Me.btStart.TabIndex = 11
        Me.btStart.Text = "Start"
        Me.btStart.UseVisualStyleBackColor = True
        '
        'lbLog
        '
        Me.lbLog.FormattingEnabled = True
        Me.lbLog.Location = New System.Drawing.Point(12, 190)
        Me.lbLog.Name = "lbLog"
        Me.lbLog.Size = New System.Drawing.Size(776, 251)
        Me.lbLog.TabIndex = 10
        '
        'btClear
        '
        Me.btClear.Location = New System.Drawing.Point(713, 161)
        Me.btClear.Name = "btClear"
        Me.btClear.Size = New System.Drawing.Size(75, 23)
        Me.btClear.TabIndex = 19
        Me.btClear.Text = "Clear"
        Me.btClear.UseVisualStyleBackColor = True
        '
        'btRead
        '
        Me.btRead.Enabled = False
        Me.btRead.Location = New System.Drawing.Point(511, 65)
        Me.btRead.Name = "btRead"
        Me.btRead.Size = New System.Drawing.Size(75, 23)
        Me.btRead.TabIndex = 18
        Me.btRead.Text = "Read"
        Me.btRead.UseVisualStyleBackColor = True
        '
        'btSend
        '
        Me.btSend.Enabled = False
        Me.btSend.Location = New System.Drawing.Point(713, 65)
        Me.btSend.Name = "btSend"
        Me.btSend.Size = New System.Drawing.Size(75, 23)
        Me.btSend.TabIndex = 17
        Me.btSend.Text = "Send"
        Me.btSend.UseVisualStyleBackColor = True
        '
        'edData
        '
        Me.edData.Location = New System.Drawing.Point(511, 39)
        Me.edData.Name = "edData"
        Me.edData.Size = New System.Drawing.Size(277, 20)
        Me.edData.TabIndex = 16
        '
        'laDataToSend
        '
        Me.laDataToSend.AutoSize = True
        Me.laDataToSend.Location = New System.Drawing.Point(437, 42)
        Me.laDataToSend.Name = "laDataToSend"
        Me.laDataToSend.Size = New System.Drawing.Size(68, 13)
        Me.laDataToSend.TabIndex = 15
        Me.laDataToSend.Text = "Data to send"
        '
        'btDisconnect
        '
        Me.btDisconnect.Enabled = False
        Me.btDisconnect.Location = New System.Drawing.Point(356, 10)
        Me.btDisconnect.Name = "btDisconnect"
        Me.btDisconnect.Size = New System.Drawing.Size(75, 23)
        Me.btDisconnect.TabIndex = 14
        Me.btDisconnect.Text = "Disconnect"
        Me.btDisconnect.UseVisualStyleBackColor = True
        '
        'fmMain
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(800, 450)
        Me.Controls.Add(Me.lvDevices)
        Me.Controls.Add(Me.btStop)
        Me.Controls.Add(Me.btStart)
        Me.Controls.Add(Me.lbLog)
        Me.Controls.Add(Me.btClear)
        Me.Controls.Add(Me.btRead)
        Me.Controls.Add(Me.btSend)
        Me.Controls.Add(Me.edData)
        Me.Controls.Add(Me.laDataToSend)
        Me.Controls.Add(Me.btDisconnect)
        Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle
        Me.Name = "fmMain"
        Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen
        Me.Text = "Multy GATT Client Test"
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub

    Private WithEvents lvDevices As ListView
    Private WithEvents chAddress As ColumnHeader
    Private WithEvents chName As ColumnHeader
    Private WithEvents chStatus As ColumnHeader
    Private WithEvents btStop As Button
    Private WithEvents btStart As Button
    Private WithEvents lbLog As ListBox
    Private WithEvents btClear As Button
    Private WithEvents btRead As Button
    Private WithEvents btSend As Button
    Private WithEvents edData As TextBox
    Private WithEvents laDataToSend As Label
    Private WithEvents btDisconnect As Button
End Class
