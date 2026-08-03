# DNA Orbit 意思決定ログ

## Decision ID

`ADR-002`

## 日付

2026-07-30

## 状態

- Accepted

## 題名

Factory presetを全12パラメータ定義型に変更し、エディタ状態をオーディオ状態から分離する

## 背景

Phase 0までのfactory preset(`PluginEditor.cpp`のローカル`Preset`構造体)は
8フィールド(`rate/radius/depth/symmetry/twist/core/mix/nullCore`)しか
持たず、`sync/division/output/autoGain`の4パラメータには一切触れていな
かった。そのため「プリセットを選ぶ」という操作の結果が、選ぶ前にユーザーが
何を触っていたかに依存し、非決定的だった(ロードマップの言う
「factory presetを全音響パラメータ定義型に変更」が未達)。

また、エディタのタブ選択・ウィンドウサイズ(`editorPage`/`editorWidth`/
`editorHeight`)はAPVTSの`state`ルートに直接フラットなプロパティとして
書かれており、オーディオに影響するパラメータ状態と混在していた。

## 選択肢

### A: 何もしない

非決定的なプリセット、フラットなUI状態を維持。

### B: Presetを`Source/Presets.h`に抽出し全12パラメータを設定。UI状態を
`uiState`子ノードへ分離しレガシー移行を実装(採用)

### C: フルのプリセットシステム(Save-As、A/Bコンペア、Undo/Redo)を
今回一緒に実装する

Phase 5で明示的にスコープされている範囲であり、今回は見送り。

## 決定

**B**。

- `Source/Presets.h`: `Preset`構造体を12フィールドに拡張し、
  `apply()`が毎回全パラメータを`setValueNotifyingHost`する。
  `matchesCurrentState()`で「プリセットから変更されたか」を判定し、
  軽量なModified/Revertインジケータ(Save-As/A-B/Undo-Redoは対象外、
  Phase 5へ委譲)をUIに追加した。
- エディタ状態は`apvts.state`の子ノード`uiState`に移動。
  過去に保存されたフラットな`editorPage`/`editorWidth`/`editorHeight`は
  `PluginProcessor::setStateInformation`内の`migrateLegacyUiState()`で
  自動的に`uiState`へ移し替え、ルートから削除する。

## 理由

- 音質: 影響なし(UIのみの変更)。
- 互換性: 既存プロジェクトのパラメータIDは一切変更していない。
  レガシーのフラットUIプロパティも移行するため、既存プロジェクトの
  タブ選択・ウィンドウサイズがサイレントに失われることはない。
- UX: プリセットの結果が常に同じになる(決定性)。Modified/Revertで
  「今の音がプリセットから変わっているか」が視覚的にわかる。
- 実装難度: Presetロジックをヘッダに抽出したことでGUIなしにテスト可能。
- テスト可能性: `Tests/PresetsTests.cpp`で決定性・Modified判定・Revertを
  GUI非依存でユニットテストできる。

## 影響

### 良い影響

- プリセット選択が完全に決定的になる。
- UI状態とオーディオ状態が明確に分離され、将来のツール(diff/migration)が
  UI状態を誤ってパラメータとして扱う心配がない。
- Modified/Revertでユーザーが「プリセットから変えた」ことに気付きやすい。

### 悪い影響

- なし(いずれも既存の保存互換性を壊さない追加的な変更)。

### 互換性

- パラメータID変更なし。
- 過去に保存された`editorPage`/`editorWidth`/`editorHeight`はロード時に
  自動移行される(`Tests/StateTests.cpp`のマイグレーションテストで検証)。

## 実装

- file: `Source/Presets.h` (新規), `Source/PluginEditor.{h,cpp}`,
  `Source/Parameters.h` (`uiStateNodeID`等の定数),
  `Source/PluginProcessor.cpp` (`migrateLegacyUiState`)
- migration: `PluginProcessor::setStateInformation`が起動時に一度実行

## テスト

- unit: `Tests/PresetsTests.cpp`(決定性、Modified判定、Revert)、
  `Tests/StateTests.cpp`(レガシーUI状態の移行、移行なしケースの非クラッシュ)
- manual: Xvfbスクリーンショットで基本/詳細タブとプリセット欄のレイアウトを
  目視確認(`shot_basic_locked.png`/`shot_detail_locked.png`)

## 再検討条件

Phase 5でSave-As/A-B/Undo-Redoを実装する際、Modified/Revertの実装を
そのまま拡張するか置き換えるかをそこで再検討する。
