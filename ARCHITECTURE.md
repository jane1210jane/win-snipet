# Windows 11用スニペットツール アーキテクチャ・基本設計書

## 1. 文書情報

| 項目 | 内容 |
|---|---|
| 対象 | 初期リリース |
| 入力文書 | `REQUIREMENTS.md` |
| 対象OS | Microsoftのサポート対象である64-bit版Windows 11 |
| アーキテクチャ | 単一プロセス、イベント駆動、レイヤードアーキテクチャ |
| 実装方式 | C++20、Unicode版Win32 API、Windows SDK、標準C++ライブラリ |
| 外部依存 | なし。OS標準APIと採用言語の標準ライブラリだけを使用 |

本書は実装時の基準である。未承認の外部ライブラリ、runtime、パッケージ、バンドラ、インストーラ作成ツールは追加しない。追加が必要になった場合は `REQUIREMENTS.md` 7章の承認手続きを先に行う。

## 2. 設計方針と決定事項

### 2.1 技術選定

初期版はC++20とWin32 APIで実装する。GUIはWin32標準コントロール、トレイ常駐はShell API、ホットキーは `RegisterHotKey`、文字入力は `SendInput`、設定はWindows標準のMSXML 6.0 COM APIを使用する。

| 候補 | 配布時runtime | 軽量性 | GUI実装性 | 判定 |
|---|---:|---:|---:|---|
| C++20 + Win32 | 不要 | 高い | 実装量は多い | 採用 |
| C# + WPF | .NET runtimeまたは自己完結runtimeが必要 | 中程度 | 高い | 初期版では不採用 |
| WinUI 3 | Windows App SDK同梱が必要 | 中程度 | 高い | 初期版では不採用 |

採用理由は、単一EXE、通常ユーザ権限、1プロセス、低い待機時負荷、および外部依存なしという要件への適合性である。NFR-01の具体値は実装後にReleaseビルドで計測し、本書15章の暫定値を確定する。

### 2.2 実装前未確定事項に対する初期版の決定

| No. | 決定 |
|---:|---|
| 1 | 1スニペットにつき1つのグローバルショートカットとする。 |
| 2 | Ctrl、Alt、Shift、Windowsキーの1つ以上と、英数字、F1～F24、OEM記号キー、ナビゲーションキーの組合せを許可する。修飾キー単独、Esc、Tab、Enter、Space、Backspace、Delete、PrintScreen、Pause、CapsLock、NumLock、ScrollLock、およびOS予約キーは割当不可とする。Windowsキーを含む組合せはOS予約との競合が多い旨をGUIに表示する。 |
| 3 | クリップボードを使用しない。`SendInput` によりUnicode文字を直接入力するため、退避・復元および同時更新競合は発生しない。 |
| 4 | パスワード欄を技術的に確実に識別できないため入力を許可する。設定画面に「機密入力欄にも入力され得る」旨を表示する。アプリは入力先の内容を読み取らない。 |
| 5 | XML形式で平文保存する。機密情報を保存しないよう初回表示と編集画面に注意書きを出す。 |
| 6 | 要件どおり、初回正常起動時に無条件で現在ユーザのスタートアップへ登録する。初回GUIで実施結果と解除方法を通知する。 |
| 7 | 初期版はポータブルな単一EXEとする。インストーラは提供しないため、削除前にGUIの「スタートアップ登録を解除」を実行する。将来インストーラを提供する場合はアンインストール処理で同じ登録を削除する。 |
| 8 | 初期版のコード署名は別途判断とする。未署名配布時の警告をリリースノートに記載する。 |
| 9 | 暫定受入値を15章に定義し、比較計測後に確定する。 |
| 10 | Notepad、Microsoft Edgeの通常入力欄、Microsoft Office系アプリの3系統をE2E対象候補とする。管理者権限プロセスへの入力は保証しない。 |

`SendInput` は一般的な編集コントロールでは貼り付け相当の結果を得られるが、アプリ固有の入力処理、ゲーム、リモートデスクトップ、管理者権限プロセスでは動作を保証しない。実装スパイクで受入対象3アプリを検証し、不適合の場合に限りクリップボード方式を再設計する。

## 3. システム構成

```text
Windows user session
└─ SnippetTool.exe (single process / single UI thread)
   ├─ Presentation
   │  ├─ Main window / edit dialog / confirmation dialog
   │  └─ Task tray icon and menu
   ├─ Application
   │  ├─ SnippetService
   │  ├─ HotkeyCoordinator
   │  └─ ApplicationLifecycle
   ├─ Domain
   │  ├─ Snippet / Hotkey / AppSettings
   │  └─ validation and uniqueness rules
   └─ Infrastructure
      ├─ Win32HotkeyRegistry
      ├─ SendInputTextEmitter
      ├─ XmlSettingsRepository
      ├─ RegistryStartupManager
      ├─ NamedMutexSingleInstance
      └─ FileLogger
```

プロセスはGUIスレッドのWindowsメッセージループで待機する。アプリ独自の常駐ワーカースレッド、ポーリング、サービス、補助プロセスは作成しない。MSXML等が内部で作るOS/runtime管理スレッドは計測対象に含める。

## 4. レイヤーと依存関係

依存方向は `Presentation -> Application -> Domain` とし、InfrastructureはApplicationが定義する抽象インターフェースを実装する。DomainはWin32型、HWND、レジストリ、XMLを参照しない。

| コンポーネント | 責務 | 主なインターフェース |
|---|---|---|
| `SnippetService` | CRUD、検証、保存単位の制御 | `List`, `Add`, `Update`, `Remove` |
| `HotkeyCoordinator` | 設定とOS登録の同期、競合の局所化 | `ApplyAll`, `Replace`, `UnregisterAll` |
| `ApplicationLifecycle` | 起動、二重起動通知、終了順序 | `Start`, `ShowMainWindow`, `Shutdown` |
| `IHotkeyRegistry` | グローバルホットキー登録の抽象化 | `Register`, `Unregister` |
| `ITextEmitter` | 現在のフォーカス先への文字入力 | `Emit` |
| `ISettingsRepository` | 設定の読込・原子的保存 | `Load`, `Save` |
| `IStartupManager` | HKCU Run登録の照会・更新・削除 | `Get`, `Enable`, `Disable`, `Repair` |
| `ISingleInstance` | 同一ユーザセッションでの排他と既存画面通知 | `Acquire`, `NotifyExisting` |

Infrastructureから上位層へは、Win32エラーコードをそのまま漏らさず、分類済みのエラーと診断コードを返す。ログには診断コード、API名、OSエラーコードを記録できるが、本文や入力先情報は記録しない。

## 5. ドメインモデル

### 5.1 Snippet

| フィールド | 型 | 制約 |
|---|---|---|
| `id` | UUID文字列 | 作成時にGUIDを生成、不変、一意 |
| `name` | UTF-16文字列 | 前後空白除去後1～100文字 |
| `body` | UTF-16文字列 | 1～32,768 UTF-16コード単位、改行は保存時にLFへ正規化 |
| `hotkey` | `Hotkey` | 有効時は必須、許可キー規則を満たす |
| `enabled` | bool | 既定値true |

名称の重複は許可する。ホットキーは大文字小文字および左右修飾キーを区別せず、正規化した `(modifiers, virtualKey)` が全スニペットで一意でなければならない。無効スニペット間も重複を禁止し、再有効化時の曖昧さを防ぐ。

### 5.2 Hotkey

`modifiers` はCtrl、Alt、Shift、Windowsのビット集合、`virtualKey` は単一の仮想キーコードとする。表示順は `Ctrl+Alt+Shift+Win+Key` に固定する。登録時には `MOD_NOREPEAT` を必ず付与し、キーリピートによる重複入力を防ぐ。

### 5.3 AppSettings

| フィールド | 内容 |
|---|---|
| `schemaVersion` | 初期値1 |
| `startupEnabled` | GUI表示用の希望値。実際のレジストリ値を起動時に照合する |
| `firstRunCompleted` | 初回通知済みか |
| `snippets` | 0件以上のSnippet |

## 6. 起動・多重起動設計

1. `Local\\SnippetTool-{UserSid}` という名前付きMutexを `CreateMutexW` で作成する。名前にユーザSIDを含め、ユーザセッション内の分離を明確にする。
2. 先行インスタンスがある場合、登録済みのウィンドウメッセージを `HWND_BROADCAST` へ送信する。先行インスタンスは設定画面を復元し、`SetForegroundWindow` が許す範囲で前面化する。後続プロセスは終了する。
3. 先行インスタンスは非表示のトップレベルメッセージウィンドウを早期に作成し、二重起動通知と `TaskbarCreated` を受信する。
4. 設定を読み込み、成功したスニペットだけホットキー登録する。個別失敗は他の登録を妨げない。
5. 初回正常起動ならHKCU Runへ登録する。以後、起動時に有効設定なら現在のEXEパスへ修復する。
6. トレイアイコンを作成し、設定画面を表示する。スタートアップ起動引数 `--background` の場合は非表示で開始する。

Mutexはプロセスハンドルのクローズまたはプロセス終了でOSが解放するため、異常終了後に永続ロックは残らない。Mutex名のSID以外に秘密情報は含めない。

## 7. ホットキー登録と更新

OS登録IDは起動中だけ有効な整数とし、`id -> snippet UUID` の表を保持する。`WM_HOTKEY` を受信したら、現在の有効モデルからUUIDを再解決して本文を1回だけ送る。

設定変更時は次の順序で反映する。

1. 入力値とアプリ内重複を検証する。
2. 新規または変更後ホットキーを仮の登録IDで `RegisterHotKey` する。
3. 登録成功後に旧ホットキーを解除し、対応表とモデルを切り替える。
4. 設定ファイルを原子的に保存する。
5. 保存失敗時は可能な範囲で旧登録へロールバックし、画面上の未保存状態を維持する。

OS競合時は編集を保存せず、「`Ctrl+Alt+X` はWindowsまたは他のアプリが使用しているため登録できません」のように対象キーを表示する。Win32 APIは競合元を特定できないため、他アプリ名は表示しない。起動時の個別登録失敗は一覧行にエラー状態を表示し、他のホットキーは稼働させる。

## 8. 文字入力設計

`WM_HOTKEY` 受信時点で別アプリが前面であることを確認し、本文をUTF-16コード単位列として `SendInput` へ渡す。各コード単位を `KEYEVENTF_UNICODE` のkeydown/keyupペアに変換する。LFはEnterキーのkeydown/keyupへ変換する。入力配列は一括送信し、戻り値が要求イベント数未満なら失敗とする。

ホットキーの物理修飾キーが解放される前に文字送信すると入力先がショートカットとして解釈する可能性がある。このため、`WM_HOTKEY` 受信後に入力対象HWNDを記録し、短いワンショットタイマーで全修飾キーの解放を最大500 msまで確認する。10 ms間隔の継続ポーリングは行わず、`WM_TIMER` による最大50回の有界処理とし、完了または期限到達で必ず停止する。期限到達時は入力せず通知する。入力前に前面HWNDが変わった場合も誤送信防止のため中止する。

1回の本文送信中は再入を禁止する。入力イベントには `dwExtraInfo` の固定識別値を付与する。グローバルキーフックは使用せず、同じホットキーの再発火は `MOD_NOREPEAT` と再入ガードで防止する。長文入力中にユーザ操作と混在し得ることは既知の制約とし、最大本文長を制限する。

UIPIにより、通常権限の本アプリから管理者権限プロセスへの `SendInput` は失敗し得る。昇格再起動やサービスは提供しない。

## 9. GUI設計

### 9.1 メインウィンドウ

- 一覧列: 名称、ショートカット、有効状態、登録状態
- 操作: 新規、編集、削除、有効／無効、スタートアップ設定、スタートアップ登録解除、終了
- ステータス領域: 読込エラー、ホットキー競合、保存失敗、スタートアップ登録結果
- 閉じるボタン: ウィンドウを非表示にし、「タスクトレイで実行中」を初回のみ通知する

### 9.2 編集ダイアログ

- 名称の単一行入力
- 本文の複数行入力と文字数表示
- 「ショートカットを入力」コントロール
- 有効チェックボックス
- 平文保存および機密入力欄にも送信され得る旨の注意書き
- 項目直下の検証エラー。最初のエラーへフォーカスする

削除は名称を含む確認ダイアログを表示する。設定読込に失敗した場合は破損ファイルを上書きせず、空の一時モデルで設定画面を開く。「再試行」「破損ファイルを退避して初期化」「終了」を提供する。初期化は明示確認後だけ実行する。

### 9.3 タスクトレイ

左ダブルクリックで設定画面を表示する。右クリックメニューは「設定を開く」「終了」を持つ。`TaskbarCreated` メッセージを受信したら `Shell_NotifyIconW(NIM_ADD)` でアイコンを再登録する。

## 10. 設定保存設計

### 10.1 配置

| 種類 | パス |
|---|---|
| 実行ファイル | ユーザが配置した任意のディレクトリ |
| 設定 | `<EXEフォルダー>\\settings.xml` |
| 一時設定 | `<EXEフォルダー>\\settings.xml.tmp` |
| バックアップ | `<EXEフォルダー>\\settings.xml.bak` |
| ログ | `<EXEフォルダー>\\logs\\app.log` |

設定ディレクトリは `GetModuleFileNameW` で取得した実行ファイルの親ディレクトリとし、カレントディレクトリには依存しない。

### 10.2 XML形式

```xml
<?xml version="1.0" encoding="utf-8"?>
<snippetTool schemaVersion="1" startupEnabled="true" firstRunCompleted="true">
  <snippets>
    <snippet id="{GUID}" name="挨拶" enabled="true">
      <hotkey modifiers="Ctrl|Alt" virtualKey="72" />
      <body>こんにちは。&#10;よろしくお願いします。</body>
    </snippet>
  </snippets>
</snippetTool>
```

MSXMLは外部エンティティ解決、DTD、XSLTを無効化する。未知のschemaVersionは読込を拒否し、元ファイルを保持する。同じversionの未知要素・属性は将来互換性のため無視する。必須値欠落、上限超過、UUID不正、重複ホットキーは項目単位で報告する。

### 10.3 原子的保存

1. 同一ディレクトリの `.tmp` へXMLを書き、ファイルバッファを `FlushFileBuffers` する。
2. 現行ファイルがある場合は `ReplaceFileW(settings, tmp, bak, ...)` で置換する。
3. 初回保存は `.tmp` を `MoveFileExW(..., MOVEFILE_WRITE_THROUGH)` で本番名へ移動する。
4. 失敗時は現行ファイルを維持し、`.tmp` を診断用に残すか安全に削除する。

読込時は本番ファイルだけを自動採用する。`.bak` からの復旧は内容を検証し、ユーザ確認後に行う。

## 11. スタートアップ設計

`HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run` の値名 `SnippetTool` に、引用符で囲んだ絶対EXEパスと `--background` をREG_SZで保存する。書込前後に値を読み戻して検証する。設定が有効で値が不在またはパス不一致なら手動起動時に修復する。設定が無効なら本アプリが所有する値だけを削除する。

ポータブル版にはOSのアンインストールイベントがない。このため「スタートアップ登録を解除」操作を常設し、配布文書にEXE削除前の実行を明記する。インストーラ採用時は同じ値名・所有確認規則で削除する。

## 12. エラー処理とログ

ユーザが対処可能なエラーはGUIに日本語で表示し、詳細診断コードを併記する。致命的エラーでもコンソールは表示しない。予期しない例外はUIメッセージ境界で捕捉し、可能ならホットキーを解除して終了する。

ログは日付、レベル、イベントID、診断コード、Win32エラーコードだけを記録する。スニペット名、本文、クリップボード、キー入力先ウィンドウのタイトル・プロセス名は記録しない。1ファイル1 MiB、最大3世代でローテーションし、テレメトリ送信は行わない。

代表イベントIDは次のとおりとする。

| ID | 事象 |
|---|---|
| `CFG-LOAD-001` | 設定XMLの解析失敗 |
| `CFG-SAVE-001` | 原子的置換失敗 |
| `HOTKEY-REG-001` | OSホットキー競合・登録失敗 |
| `INPUT-SEND-001` | SendInputの部分失敗 |
| `STARTUP-REG-001` | HKCU Runの更新失敗 |
| `TRAY-ADD-001` | トレイアイコン登録失敗 |

## 13. セキュリティ設計

- ネットワークAPIを使用せず、外部送信を行わない。
- 本文は平文であるため、UIと配布文書で機密情報の保存を非推奨とする。
- 設定ファイルは実行ファイルと同じフォルダーに保存し、そのフォルダーの継承ACLを利用する。書き込み可能なフォルダーへ配置することを前提とする。
- XML外部実体を無効化し、XXEおよび外部参照を防ぐ。
- `SendInput` の送信先がホットキー受信時から変化した場合は中止し、誤送信を優先して防ぐ。
- EXEパスをレジストリへ格納する際は必ず引用し、コマンドライン解釈によるパス分割を防ぐ。
- UIプロセスを昇格せず、`uiAccess` は要求しない。

## 14. ソース構成とビルド

```text
/
├─ REQUIREMENTS.md
├─ ARCHITECTURE.md
├─ SnippetTool.sln
├─ src/
│  ├─ app/              # WinMain、lifecycle、composition root
│  ├─ domain/           # モデル、検証、値オブジェクト
│  ├─ application/      # ユースケース、抽象インターフェース
│  ├─ infrastructure/   # Win32、XML、registry、file実装
│  ├─ presentation/     # window、dialog、tray、resource
│  └─ SnippetTool.vcxproj
└─ tests/
   ├─ unit/             # OS非依存テスト
   ├─ integration/      # Win32アダプタとファイルのテスト
   └─ e2e/              # 別プロセスのテスト用入力先とUI Automation
```

Visual StudioのMSVC、Windows SDK、MSBuildを開発ツールとして使用する。Releaseはx64、`/W4 /WX /permissive- /analyze /guard:cf /DYNAMICBASE /NXCOMPAT /CETCOMPAT` を有効にし、C++例外はアプリ境界で処理する。CRTは単一EXE要件に合わせ静的リンク (`/MT`) とする。GUI subsystem (`/SUBSYSTEM:WINDOWS`) を使用する。

テストは外部テストフレームワークを追加せず、標準C++の小さなテストランナーをリポジトリ内に実装する。テスト失敗は非0終了コードとし、MSBuild/CIから実行する。

## 15. テスト戦略と品質基準

### 15.1 TDDとテスト層

- ユニット: バリデーション、ホットキー正規化・重複、CRUD、XML DTO変換、ロールバック判断
- 結合: `RegisterHotKey` 登録解除、Mutex、HKCUのテスト専用値、原子的保存、破損XML、SendInputによるテスト用ウィンドウ入力
- E2E: GUI CRUD、再起動保持、二重起動、トレイ復元、スタートアップ切替、対象3アプリへの日本語・記号・複数行入力
- 静的解析: MSVC `/analyze`、`/W4 /WX`

OS状態を変更する結合テストは専用のMutex名、レジストリ値名、設定ディレクトリを使用し、終了処理で後始末する。実ユーザ設定を参照・変更しない。各テスト名またはメタデータに要件IDを含める。

### 15.2 暫定性能受入値

比較計測前の目標値を次のように置く。Release x64を、対象Windows 11のログイン後5分以上経過した環境で3回測定し、中央値を採用する。

| 指標 | 暫定値 | 測定方法 |
|---|---:|---|
| アイドルCPU | 60秒平均0.1%未満 | Process CPUのサンプリング。操作なし、ホットキー10件 |
| Private Working Set | 30 MiB以下 | 起動60秒後 |
| プロセス内スレッド数 | 4以下 | 起動60秒後。OSコンポーネント生成分を含む |
| 起動時間 | 500 ms以下 | プロセス開始からトレイ登録完了まで |
| ホットキー応答 | 100 ms以下を目標 | 押下から最初の文字入力まで。修飾キー解放待ちは除外して別記 |

実測で未達の場合、原因とユーザ影響を記録したうえで最適化または基準変更をレビューする。基準を無断で緩和しない。

## 16. 主要シーケンス

### 16.1 スニペット保存

```text
User -> EditDialog: 保存
EditDialog -> SnippetService: Add/Update(draft)
SnippetService -> DomainValidator: validate
SnippetService -> HotkeyCoordinator: Replace(old, new)
HotkeyCoordinator -> RegisterHotKey: register new
SnippetService -> SettingsRepository: Save(all settings)
SettingsRepository -> FileSystem: tmp + flush + atomic replace
SnippetService -> EditDialog: success
```

登録または保存のどちらかが失敗した場合、ダイアログは閉じない。登録成功後の保存失敗ではホットキー登録を旧状態へ戻し、GUIモデルと永続データの一致を保つ。

### 16.2 ホットキー入力

```text
Windows -> MessageWindow: WM_HOTKEY(id)
MessageWindow -> HotkeyCoordinator: resolve(id)
HotkeyCoordinator -> InputGuard: capture foreground HWND / reject reentry
InputGuard -> MessageLoop: bounded modifier-release timer
InputGuard -> TextEmitter: Emit(body) if HWND unchanged
TextEmitter -> Windows: SendInput(KEYEVENTF_UNICODE)
TextEmitter -> UI/Log: result without body
```

## 17. 要件トレーサビリティ

| 要件 | 主な設計箇所 | 主な検証 |
|---|---|---|
| FR-01 | 3, 6, 9 | 常駐・プロセス・アイドル計測 |
| FR-02 | 5, 9 | CRUDユニット/E2E |
| FR-03 | 5, 7 | 重複ユニット、競合結合 |
| FR-04 | 8 | Unicode/複数行/失敗結合・E2E |
| FR-05 | 9 | GUI E2E |
| FR-06 | 10 | 原子的保存・破損XML結合 |
| FR-07 | 6 | 並行起動E2E、異常終了結合 |
| FR-08 | 11 | HKCU結合、サインイン相当E2E |
| FR-09 | 6, 7, 12 | 終了・障害注入テスト |
| NFR-01 | 2, 3, 15 | 性能計測 |
| NFR-02 | 7, 8, 9 | 再入、部分競合、Explorer再起動E2E |
| NFR-03 | 10, 12, 13 | ログ検査、ネットワークAPIレビュー |
| NFR-04 | 4, 14, 15 | CI全テスト、静的解析 |
| NFR-05 | 10, 11, 14 | Release成果物検査 |

## 18. 実装順序

1. Domainモデル、検証、テストランナー
2. XMLリポジトリと原子的保存
3. Mutexと既存インスタンス通知
4. メッセージウィンドウ、ホットキー登録、入力スパイク
5. CRUD GUIとトレイ
6. スタートアップ、ログ、障害処理
7. 結合/E2E、性能計測、対象アプリ互換性確認
8. 配布物検査と受入試験

各作業はGitHub Issueに要件IDと完了条件を記載し、Issue単位のブランチとプルリクエストで進める。
