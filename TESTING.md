# テストと要件トレーサビリティ

テスト名に要件IDを含める。Releaseゲートは次の順序で実行する。

    msbuild SnippetTool.sln /p:Configuration=Release /p:Platform=x64
    .\x64\Release\SnippetToolTests.exe
    powershell -ExecutionPolicy Bypass -File .\tests\e2e\run_smoke.ps1
    powershell -ExecutionPolicy Bypass -File .\tests\e2e\run_compatibility.ps1

| 要件 | 自動検証 |
|---|---|
| FR-01 | E2Eの常駐、トレイへのクローズ、既存画面復元、性能測定 |
| FR-02 | ドメイン検証、サービスCRUD、GUIで10件CRUDと再起動保持 |
| FR-03 | キー規則、重複、Fキー境界、サービス失敗、実OS登録 |
| FR-04 | INPUT列、実編集コントロール、3種類の実アプリへのUnicode入力 |
| FR-05 | GUI CRUD、登録状態、スタートアップチェック、トレイ復元 |
| FR-06 | MSXML往復、特殊文字、破損・未知schema、バックアップ、原子的置換 |
| FR-07 | 名前付きMutex、後続プロセス終了、先行プロセス保持 |
| FR-08 | テスト専用HKCU値で登録・解除・所有確認、パス引用 |
| FR-09 | ローテーションログ、破損設定保持、終了時解除 |
| NFR-01 | 10件登録時の3回中央値: 起動45 ms、Working Set 12.60 MiB、4スレッド、アイドルCPU 0 ms/2秒 |
| NFR-02 | MOD_NOREPEAT、再入ガード、個別競合継続、TaskbarCreated処理 |
| NFR-03 | 本文を受け取らないロガー、ネットワークDLL依存なし |
| NFR-04 | 外部テストFWなし、W4/WX/analyze、単体・結合・E2E |
| NFR-05 | x64 Windows GUI、静的CRT、単一起動EXE |

性能値は同一環境で3回測定し、中央値が設計書15.2の暫定値以内であることを確認する。互換テストは隔離設定を使用し、Notepad、PowerShell ISE、Explorerを終了後に後始末する。
