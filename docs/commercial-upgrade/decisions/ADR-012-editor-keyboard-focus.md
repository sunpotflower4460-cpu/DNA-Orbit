# DNA Orbit 意思決定ログ

## Decision ID

`ADR-012`

## 日付

2026-08-03

## 状態

- Accepted

## 題名

`EDITOR_WANTS_KEYBOARD_FOCUS` を `FALSE` から `TRUE` に修正する
(Undo/Redoがプラグイン版で一度も機能していなかった実バグ)

## 背景

ADR-009でUndo/Redo(Ctrl+Z / Ctrl+Shift+Z)を実装した際、
`PluginEditor`に`setWantsKeyboardFocus (true)`と`keyPressed()`の
オーバーライドを追加した。ユニットテストではAPVTSの`UndoManager`が
正しく配線されていることを確認し(`Tests/UndoRedoTests.cpp`)、
機能自体は正しく実装されていると判断していた。

PR #2(`agent/world-class-dsp-phase3`)を精査した際、そのブランチの
`CMakeLists.txt`が`juce_add_plugin`に
`EDITOR_WANTS_KEYBOARD_FOCUS TRUE`を指定していることに気づいた。
本ブランチ(main)は当初から`FALSE`のままだった。

## 問題

JUCEのソースを確認したところ、`EDITOR_WANTS_KEYBOARD_FOCUS`は
`JucePlugin_EditorRequiresKeyboardFocus`というプリプロセッサマクロに
変換され、各プラグインフォーマットのラッパーがエディタウィンドウを
デスクトップに追加する際のフラグを決定する。macOS実装
(`detail/juce_VSTWindowUtilities.h`、VST3/AUで共有)を見ると:

```cpp
const auto defaultFlags = JucePlugin_EditorRequiresKeyboardFocus
                        ? 0
                        : ComponentPeer::windowIgnoresKeyPresses;
comp->addToDesktop (desktopFlags | defaultFlags, parentView);
```

`FALSE`(既定値)のときは`windowIgnoresKeyPresses`が付与され、
**エディタウィンドウはキーイベントを一切受け取らない**。
これはコンポーネント側で`setWantsKeyboardFocus(true)`をいくら
呼んでも無効化できない、ウィンドウ生成時点でのOS/ホストレベルの
設定である。

結果として、Ctrl+Z / Ctrl+Shift+Zは**Standalone版でのみ**機能し
(Standaloneはこのフラグの影響を受けない独自のウィンドウ管理を持つ)、
実際にDAWにVST3/AUとしてロードした場合は`PluginEditor::keyPressed()`が
一度も呼ばれない。ユニットテストは`processor.undoManager`と
`apvts`の配線を検証しているが、実際のキーイベント配送経路
(OS→ホスト→JUCEラッパー→Component)は検証範囲外だったため、
この不具合はテストでは検出できなかった。

## 決定

`CMakeLists.txt`の`juce_add_plugin`呼び出しで
`EDITOR_WANTS_KEYBOARD_FOCUS`を`TRUE`に変更する。

## 代償とトレードオフ

このフラグをTRUEにすると、エディタウィンドウがフォーカスを
持っている間、プラグインが消費しないキー入力もホスト側の
ショートカット(スペースキーでの再生/停止など)に届かない
可能性がある — `windowIgnoresKeyPresses`が外れることで、
ウィンドウは「キーを受け取れる」状態になるが、その後の転送は
ホストの実装次第になる。

これを緩和するため、`PluginEditor::keyPressed()`は
Undo/Redoの2つのキー組み合わせ以外はすべて`false`を返す
(=「処理しなかった」とJUCEに伝える)設計に既になっている。
JUCEの通常のキーイベント伝播では、`keyPressed()`が`false`を
返した場合にホストへ転送されるかどうかは、最終的にはホストの
実装(VST3/AUホストがJUCEの「未処理キー」をどう扱うか)に依存する。
これは実DAWでの確認が必要な項目として`MANUAL_REQUIRED.md`に
追加した。

## なぜユニットテストで発見できなかったか

`Tests/UndoRedoTests.cpp`は「`apvts.undoManager`が
`processor.undoManager`を指しているか」「ValueTreeノードを直接
書き換えたときにUndo/Redoが機能するか」を検証しており、これらは
すべて正しく機能していた。しかし検証していなかったのは
「実際のキー入力がOSからプラグインのコンポーネントまで届くか」
という、ヘッドレスなユニットテスト環境では原理的に再現できない
経路である。これはウィンドウシステム・ホストプロセスに依存する
統合レベルの契約であり、`MANUAL_REQUIRED.md`が担う領域だった。

今回はPR #2の設定を実際に読んで比較したことで発見できた。
「別実装との差分を機械的に見る」ことが、抽象的なコードレビューでは
見つからなかった実バグの発見につながった一例。

## 理由

- 正しさ: Ctrl+Z/Ctrl+Shift+ZはStandalone限定ではなく、
  VST3/AU(そして将来のVST2)でも同様に機能すべき、
  ADR-009が意図した仕様。
- リスクは限定的: `keyPressed()`がUndo/Redo以外は`false`を返す
  設計により、副作用の範囲は最小化されている。

## 影響

### 良い影響

- プラグインとしてロードした場合でもUndo/Redoが機能するようになる
  (これまでStandaloneでしか機能していなかった)。

### 悪い影響

- エディタウィンドウがフォーカスを持つ間、ホストの一部キーボード
  ショートカットが期待通り動作しない可能性がある。程度はホストの
  実装次第で、このコンテナでは検証できない。

### 互換性

- パラメータID・schema・音への影響なし。プラグインフォーマットの
  ビルド設定のみの変更。

## 実装

- file: `CMakeLists.txt`(`EDITOR_WANTS_KEYBOARD_FOCUS FALSE` →
  `TRUE`、理由をコメントで明記)

## テスト

- 全111件のCTestは無改造で通過(この変更はテストバイナリの
  ビルドには影響しない、プラグインラッパー固有の設定のため)。
- VST3/Standaloneの再ビルドで生成される
  `JucePlugin_EditorRequiresKeyboardFocus`マクロが
  `1`になったことを`compile_commands.json`経由で確認。

## 再検討条件

実DAWでの試聴確認(`MANUAL_REQUIRED.md`)で、エディタフォーカス中に
ホストの重要なショートカット(特にスペースキーでの再生/停止)が
使えなくなると判明した場合、Undo/Redoをキーボードショートカットでは
なく明示的なUIボタンに置き換えることを再検討する。
