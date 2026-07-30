# DNA Orbit 意思決定ログ

## Decision ID

`ADR-001`

## 日付

2026-07-30

## 状態

- Accepted

## 題名

Bypass中もエンジン状態を進め続け、ハードクロスフェード(Soft Bypassの音量クロスフェードは見送り)

## 背景

商用アップグレードのコードレビューで指摘されたP0: `processBlockBypassed` は
`engine.*` を一切呼ばず、Bypass中は軌道位相・スムーサー・フィルタ/ディレイの
内部状態が完全に凍結していた。DAWでBypassをトグルすると、解除した瞬間に
古い位相から再開するため段差が起こりうる。3Dビジュアライザーも同じ理由で
Bypass中は静止して見える。

## 選択肢

### A: 何もしない

現状維持。P0を放置。

### B: エンジン状態だけ進める(本コミットで採用)

`processBlockBypassed` 内でスクラッチバッファに実入力をコピーし、
`engine.setParameters()` + `engine.process()` をスクラッチ側だけに対して
実行して内部状態(位相・スムーサー・フィルタ・UI用atomic)を進める。
実際に出力する `buffer` 自体はスクラッチに一切触れさせず、既存のドライ
パススルー(モノ入力のステレオ複製含む)をそのまま維持する。

### C: Soft Bypass音量クロスフェードまで実装する

`processBlock`/`processBlockBypassed` を1つの共有パスに統合し、
Bypass targetへ向けて数msでクロスフェードするスムーサーを追加する。
クリックをより積極的に潰せるが、既存の
「Bypass即座にドライと1e-6以内で一致」テストの意味を変える必要があり
(初回コールが即ドライではなく、短いランプの終了後にドライになる、という
契約変更になる)、実装・検証コストも高い。

## 決定

**B**。状態凍結というP0の実害はこれで解消する。Cのクロスフィードは
「なぜクリックするか」の根本原因(状態凍結)ではなく症状の緩和であり、
状態凍結を直すと体感クリックの主要因も同時に消えるため、優先度を下げて
Phase 1では見送る。将来的にホストのBypass実装が本当にクリックを出す
実測(実DAWでの試聴、`08_手動試聴_DAW検証仕様書.md`)が出たら、Cを
ADR-002として再提案する。

## 理由

- 音質: 状態凍結の解消は無条件の改善。追加のクロスフェードはトレードオフ
  (実装リスク・既存テスト契約の変更)を伴う。
- 互換性: 既存の「Bypass = 即座にビット透過ドライ」というテスト契約を
  一切変えずに済む。
- CPU: Bypass中もエンジン全体を(スクラッチに対して)毎ブロック実行するため、
  Bypass中のCPU使用量はアクティブ時とほぼ同じになる。これは意図的な
  トレードオフ(多くの商用プラグインの "smart bypass" と同じ設計)。
- 実装難度・テスト可能性: Bより低リスク。既存のBypassテストが無改造で通る。

## 影響

### 良い影響

- Bypass解除時に位相・フィルタ状態が連続的で、段差が出ない。
- 3Dビジュアライザーがバイパス中も動き続ける(以前は静止していた)。
- 既存のBypass関連テスト(ドライ一致)は無改造で通過。

### 悪い影響

- Bypass中のCPU使用量がアクティブ時とほぼ同等になる(単純なパススルーに
  比べて重い)。多数インスタンスを持つ大規模プロジェクトでの実測は
  `MANUAL_REQUIRED.md` の対象。

### 互換性

- パラメータID変更なし。既存プロジェクトの音・状態に影響なし。

## 実装

- file: `Source/PluginProcessor.h` (`bypassScratchBuffer` メンバ),
  `Source/PluginProcessor.cpp` (`processBlockBypassed`, `prepareToPlay`)
- migration: 不要(新しい内部スクラッチバッファのみ、保存状態に影響なし)

## テスト

- unit: `Tests/StateTests.cpp`
  - "Bypass keeps the engine's orbit phase advancing instead of freezing it"
  - "Bypass with a block larger than prepareToPlay negotiated falls back safely, still dry"
  - 既存の2件のBypassドライ一致テストは無改造で通過
- manual: 実DAWでのBypass ON/OFFトグル試聴は `MANUAL_REQUIRED.md` に記載
  (このコンテナには音声デバイスがなく、クリックの可聴確認はできない)

## 再検討条件

実DAWでの試聴(`08_手動試聴_DAW検証仕様書.md`)でBypass解除時に依然
可聴なクリック・段差が確認された場合、選択肢CをADR-002として再提案する。
