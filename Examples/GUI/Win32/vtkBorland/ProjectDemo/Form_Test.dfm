object VTK_Form: TVTK_Form
  Left = 287
  Top = 176
  Width = 610
  Height = 480
  Caption = 'VTK_Form'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = True
  OnDestroy = FormDestroy
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Panel3: TPanel
    Left = 0
    Top = 0
    Width = 101
    Height = 455
    Align = alLeft
    BevelInner = bvLowered
    BorderWidth = 2
    TabOrder = 0
    object Label1: TLabel
      Left = 10
      Top = 46
      Width = 48
      Height = 13
      Caption = 'Shrinker...'
      OnClick = BackgroundColour1Click
    end
    object bc1: TButton
      Left = 11
      Top = 8
      Width = 75
      Height = 25
      Caption = 'Create 1'
      TabOrder = 0
      OnClick = bc1Click
    end
    object ShrinkScroll: TScrollBar
      Left = 10
      Top = 62
      Width = 72
      Height = 13
      PageSize = 0
      Position = 50
      TabOrder = 1
      OnChange = ShrinkScrollChange
    end
  end
  object BorderWindow: TPanel
    Left = 101
    Top = 0
    Width = 501
    Height = 455
    Align = alClient
    BevelInner = bvLowered
    BevelWidth = 2
    BorderWidth = 2
    TabOrder = 1
    object Panel2: TPanel
      Left = 6
      Top = 432
      Width = 489
      Height = 17
      Align = alBottom
      AutoSize = True
      BevelOuter = bvNone
      TabOrder = 0
      object HeaderControl1: THeaderControl
        Left = 0
        Top = 0
        Width = 489
        Height = 17
        Align = alBottom
        DragReorder = False
        Sections = <
          item
            ImageIndex = -1
            MaxWidth = 60
            MinWidth = 60
            Text = 'Mode'
            Width = 60
          end
          item
            ImageIndex = -1
            MaxWidth = 60
            MinWidth = 60
            Text = 'Window'
            Width = 60
          end>
        OnSectionClick = HeaderControl1SectionClick
      end
    end
    object vtkWindow1: TvtkBorlandRenderWindow
      Left = 6
      Top = 6
      Width = 489
      Height = 426
      Align = alClient
      Color = clMenuText
      ParentColor = False
      TabOrder = 1
      OnEnter = vtkWindow1Enter
      OnExit = vtkWindow1Exit
    end
  end
  object ModeMenu: TPopupMenu
    Left = 193
    Top = 56
    object TrackBallMode1: TMenuItem
      Caption = 'TrackBall Mode'
      Checked = True
      GroupIndex = 1
      RadioItem = True
      OnClick = TrackBallMode1Click
    end
    object JoystickMode1: TMenuItem
      Caption = 'Joystick Mode'
      GroupIndex = 1
      RadioItem = True
      OnClick = TrackBallMode1Click
    end
    object FlightMode1: TMenuItem
      Caption = 'Flight Mode'
      GroupIndex = 1
      RadioItem = True
      OnClick = TrackBallMode1Click
    end
  end
  object WindowMenu: TPopupMenu
    Left = 221
    Top = 56
    object BackgroundColour1: TMenuItem
      Caption = 'Background Colour'
      OnClick = BackgroundColour1Click
    end
    object ResetCamera1: TMenuItem
      Caption = 'Reset Camera'
      OnClick = ResetCamera1Click
    end
  end
  object backgroundcolor: TColorDialog
    Ctl3D = True
    Left = 301
    Top = 56
  end
end
