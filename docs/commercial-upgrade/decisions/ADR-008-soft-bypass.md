# DNA Orbit 意思決定ログ

## Decision ID

`ADR-008`

## 日付

2026-07-30

## 状態

- Accepted

## 題名

プラグイン内蔵Soft Bypassを、独立パラメータ+出力段の最終クロスフェードとして実装する

## 背景

ADR-001はホストのBypass(`processBlockBypassed`)について、内部状態
(軌道位相・フィルタ・スムーサー)を凍結させない対策のみを実装し、
音量のクロスフェード自体は「症状の緩和であり根本原因ではない」として
Phase 1では見送った。しかしユーザーは後日、Phase 2.5の指摘の一部として
「Soft Bypassは未完了事項として残し、Phase 5までにプラグイン内蔵の
Soft Bypassとして実装・テストする」ことを明示的に要求した。

これはADR-001が検討した「ホストBypassのクロスフェード」とは別の要求
である: **プラグイン自身が持つ、ホストのBypassとは独立したBypass
パラメータ**。理由は主に2つ考えられる:
- ホストによってBypass自動化の質・可用性が異なる(オートメーション
  できない、UIから見えにくい、DAWごとに挙動が違う、など)。
- プラグイン内蔵のBypassなら、常にホストから見える通常のパラメータ
  として自動化・オートメーション・プリセット保存の対象になる。

## 決定

新規パラメータ `softBypass`(Bool、既定false)を追加した。
false(既定)はこのエンジンの通常処理そのものであり、既存プロジェクトの
音に一切影響しない(schemaバージョンの更新は不要 - Character/Bass
Anchor/Host Phase Lockと同じ判断基準)。

trueのときの実装は、`HelixEngine::process()`の**最終段**(Output trim
適用後、`outL[n]`/`outR[n]`へ書き込む直前)に置いた、単純なクロスフェード:

```
finalL += (dryL - finalL) * softBypassAmt;
finalR += (dryR - finalR) * softBypassAmt;
```

`softBypassAmt`は`softBypassSmoothed`(`SmoothedValue<float, Linear>`、
30ms)から毎サンプル取得する。`dryL`/`dryR`はループ冒頭で捕捉した
サニタイズ済み入力そのもの(Bass Anchor/Stereo Preserve/Mix/Output
より前)なので、`softBypassAmt == 1`のとき出力は**Mix・Output・Core・
NULL CORE設定に関わらず**入力と厳密に一致する - ホストBypassと同じ
「Bypass中は入力==出力」という契約を、プラグイン内蔵の(自動化可能な)
パラメータとして再現している。

## なぜ「最終段のクロスフェードのみ」で済むのか

`softBypassAmt`は`process()`のどこにも分岐条件として現れない
(Bass Anchor/Stereo Preserve/軌道角度更新/Mix Law等、すべて通常どおり
毎ブロック実行され続ける)。つまり:
- ADR-001がホストBypassのために作った「スクラッチバッファへの実行」
  という別経路は**不要**: Soft Bypass中も`process()`は通常の出力
  バッファに対して普通に呼ばれ続け、最後の1行だけがdryへ寄せる。
- 軌道位相・フィルタ・スムーサーは、Soft Bypassの状態に一切関係なく
  常に進み続ける。ADR-001が固定したかった性質(解除時に段差が
  出ない)を、実装をほぼ増やさずに得られる。
- `SmoothedValue<float, Linear>`は目標値に向かって線形かつ**厳密に**
  収束する(漸近ではない)ので、30ms経過後は`softBypassAmt`が
  ちょうど1.0になり、出力はDryと浮動小数点誤差の範囲で完全に一致する
  - これによって`Tests/SoftBypassTests.cpp`は「収束後は完全にDryと
    一致する」という強いアサーションを、タイムアウトや近似ではなく
    正確な収束サンプル数で検証できる。

## クロスフェード時間: 30ms

ADR-001での議論(Cの選択肢:「数msでクロスフェードする」)を踏まえつつ、
Host Phase Lock(ADR-007)の収束時定数30msと合わせて覚えやすい値とした。
クリックを確実に消すには十分長く(既存のNULL CORE切り替え用120msより
短い - Bypassは「今すぐ音を止めたい/戻したい」用途なので、120msの
モード切替よりも機敏な応答を優先)、体感の遅延が気にならない範囲。

## 選択肢として検討したが採らなかったもの

### A: ホストBypass(`processBlockBypassed`)自体にクロスフェードを追加する

ADR-001が既に検討し、既存の「即座にドライと一致する」というテスト
契約を変える必要があるため見送っている。今回のプラグイン内蔵Soft
Bypassはこれとは別のパラメータなので、ホストBypassの契約には
一切触れていない(`Tests/StateTests.cpp`の既存Bypassテストは無改造で
通過)。

### B: Wet/Dryではなく処理そのものをスキップする(CPU節約)

Bypass中はBass Anchor/軌道計算そのものをスキップしてCPUを節約する
という設計も検討したが、ADR-001と同じ理由(内部状態凍結の再発)で
不採用。Soft Bypassでも「常にフル処理してCPUを節約しない」という
ADR-001のトレードオフをそのまま踏襲する。

## 理由

- 音質: 30msの線形クロスフェードでクリックなく安全に切り替わり、
  内部状態は一切凍結しない。
- 互換性: 既定false = 従来の処理そのものなので、schemaバージョンの
  更新も移行ロジックも不要。既存の全テストは無改造で通過する。
  ホストBypassのテスト契約にも触れていない。
- 実装難度・テスト可能性: 追加コードは1つのパラメータ・1つの
  `SmoothedValue`・`process()`の最終段2行のみ。`SmoothedValue`が
  厳密に収束することを利用し、収束後の状態を正確なサンプル数で
  検証できる。

## 影響

### 良い影響

- ホストのBypass実装の質に関わらず、プラグイン自身で確実に
  クリックのないBypassを提供できる。
- 通常のパラメータなので、DAWのオートメーション・プリセット・
  ホストの汎用パラメータリストからも操作できる。

### 悪い影響

- ホストの本来のBypass(`processBlockBypassed`)とは別に、プラグイン
  UIにもうひとつ「バイパス」の概念が増える。両者の違い
  (プラグイン内蔵/ホスト側、ラベル文言含む)をユーザーが混同しない
  かは実DAWでの確認が必要(`MANUAL_REQUIRED.md`参照)。

### 互換性

- 新規パラメータID `softBypass` 追加。既存パラメータIDは変更なし。
- schemaバージョンの更新なし(既定false=従来動作のため不要)。

## 実装

- file: `Source/dsp/HelixEngine.{h,cpp}`(`softBypass`パラメータ、
  `softBypassSmoothed`、`process()`最終段のクロスフェード)、
  `Source/Parameters.h`(`softBypassID`、AudioParameterBool)、
  `Source/PluginProcessor.{h,cpp}`、`Source/PluginEditor.{h,cpp}`
  (トップバーに常時表示のトグルボタン、両タブ共通)、`Source/Presets.h`
  (全プリセットでfalseを明示)

## テスト

- unit: `Tests/SoftBypassTests.cpp`
  (既定falseが機能追加前とビット単位一致、ONで30ms以内に厳密に
  Dryへ収束しMix/Output/NULL CORE設定を無視すること、OFFに戻すと
  処理済み信号へ戻ること、Soft Bypass中も軌道位相が凍結せず進み
  続けること、クロスフェード中にサンプル間の可聴なジャンプが
  ないこと)
- 全102件のCTest + ASan/UBSanがクリーン(0エラー)

## 再検討条件

実DAWでの試聴(`MANUAL_REQUIRED.md`)で30msの時間が硬すぎる/緩すぎる
と判明した場合、またはプラグイン内蔵Bypassとホスト側Bypassの
UI上の違いがユーザーに伝わりにくいと判明した場合に見直す。
