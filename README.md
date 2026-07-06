# SnippetTool

Windows 11向けの、外部依存を持たない常駐スニペットツールです。登録したUnicodeテキストをグローバルショートカットからSendInputで入力します。

## ビルドとテスト

Visual Studioの「C++によるデスクトップ開発」とWindows SDKが必要です。

    msbuild SnippetTool.sln /p:Configuration=Release /p:Platform=x64
    .\\x64\\Release\\SnippetToolTests.exe
    powershell -ExecutionPolicy Bypass -File .\\tests\\e2e\\run_smoke.ps1
    powershell -ExecutionPolicy Bypass -File .\\tests\\e2e\\run_compatibility.ps1

成果物は x64\\Release\\SnippetTool.exe です。設定は %LOCALAPPDATA%\\SnippetTool\\settings.xml、ログは %LOCALAPPDATA%\\SnippetTool\\logs\\app.log に保存されます。

## セキュリティ上の注意

- 本文はXMLへ平文保存されます。パスワードなどの機密情報は登録しないでください。
- ショートカットは機密入力欄にも送信され得ます。
- 管理者権限で動作するアプリへの入力は保証しません。
- ネットワーク通信、テレメトリ、クリップボードは使用しません。
- EXEを削除する前にGUIで「サインイン時に起動」を無効にしてください。
