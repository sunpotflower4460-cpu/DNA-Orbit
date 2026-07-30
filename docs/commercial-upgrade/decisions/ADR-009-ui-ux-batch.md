# DNA Orbit 意思決定ログ

## Decision ID

`ADR-009`

## 日付

2026-07-30

## 状態

- Accepted

## 題名

Phase 5 UI/UXのうち、Mono PreviewとUndo/Redoを実装し、A/B比較・多言語化・
アクセシビリティ監査の範囲を明示的に決定する

## 背景

Phase 5は「UI/UX(A/B, Undo/Redo, Mono Preview, ローカライズ, アクセシビ
リティ)」という広い範囲を含む。このADRは、そのうちどれを今回実装し、
どれを理由付きで見送るかをまとめて記録する(Character §6.2やBass
Anchorの広範なCPU最適化を理由付きで一部実装に留めた、これまでの
判断パターンと同じ)。

## 実装: Mono Preview

`monoPreview`(Bool、既定false)を追加。最終出力(Soft Bypassの**後**、
つまり実際に鳴っている音そのもの)をモノラルに畳み込み、左右の打ち消し
(モノ非互換性)を試聴で確認するためのモニター専用機能。

```
monoSum = 0.5 * (finalL + finalR)
finalL += (monoSum - finalL) * monoPreviewAmount
finalR += (monoSum - finalR) * monoPreviewAmount
```

Soft Bypass(ADR-008)と全く同じ設計判断: 30msの`SmoothedValue`線形
クロスフェード、`process()`の最終段のみに置く(内部状態は一切凍結
しない)、既定false=従来動作のためschemaバージョン更新は不要。

Soft Bypassの**後**に適用する理由: Mono Preview は「今実際に聞こえて
いる音」のモノ互換性を確認する機能なので、Bypass中であればBypassされた
(Dryの)モノ互換性を、そうでなければ処理後の音のモノ互換性を、常に
一貫して試聴できるようにするため。

## 実装: Undo/Redo

`DNAOrbitAudioProcessor`に`juce::UndoManager undoManager`を追加し(
`apvts`より前に宣言 - 構築順序はメンバ宣言順に従うため)、APVTSの
コンストラクタに`nullptr`ではなく`&undoManager`を渡すよう変更した。
これによりAPVTSの標準機構(`SliderAttachment`等が内部で使う
`Attachment`は`stateToUse.undoManager`を使う)がそのままUndo/Redoに
対応する - JUCEのAPVTSベースのUndo/Redo実装として広く使われている、
素直な標準構成である。

エディタは`Ctrl+Z`(macでは`Cmd+Z`、`juce::ModifierKeys::
commandModifier`でクロスプラットフォームに対応)でUndo、
`Ctrl+Shift+Z`または`Ctrl+Y`でRedoを行う(`PluginEditor::keyPressed`)。
専用のUndo/Redoボタンは追加していない - トップバーは既にSoft Bypass/
Mono Preview追加でスペースが厳しく、キーボードショートカットは
DAWプラグインで広く使われる標準的な操作方法のため、視覚的な追加
コントロールなしで機能を提供する選択をした。ショートカットの存在は
サブタイトルのツールチップで明示している。

`Source/Presets.h`の`apply()`に`undoManagerForOneStep`という
オプション引数(既定nullptr、GUI非依存のユニットテストのため)を追加:
非nullのとき、パラメータを変更する前に一度だけ`beginNewTransaction()`
を呼ぶ。これがないと、プリセット1つの適用が(そのプリセットが触れる
パラメータの数だけ)複数回のUndoステップになってしまう。

### テスト環境の制約

`AudioProcessorValueTreeState`はパラメータ変更を内部の`ValueTree`
(UndoManagerが実際に記録する対象)へ、`setValueNotifyingHost()`と
同期的にではなく、内部の約10Hzの`juce::Timer`で反映する。この
反映(タイマー起動)にはメッセージループが必要だが、
`JUCE_MODAL_LOOPS_PERMITTED`はプラグインターゲットでは無効化されて
おり、`MessageManager::runDispatchLoopUntil()`が使えない。そのため
`Tests/UndoRedoTests.cpp`は、Undo/Redoが実際に使う`ValueTree`の
"PARAM"ノード(`Tests/StateTests.cpp`がXML往復で既に依拠している
スキーマ)を直接操作してUndo/Redoの機構そのものを検証しており、
「実際のUIドラッグ→(タイマー経由で)Undoステップとして記録される」
という前方経路のエンドツーエンドはこの環境では検証できない
(`MANUAL_REQUIRED.md`に記載)。

## 見送り: A/B比較

仕様の「A/B」(2つの設定を素早く切り替えて比較試聴する機能)は今回
実装しなかった。理由:
- 永続化の設計判断が必要 - セッション限定(プロジェクト保存に含めない)
  にするか、プロジェクトの状態に含めるかで、schemaへの影響度が
  大きく異なる。後者は今回慎重に扱ってきたschemaバージョニングの
  対象が広がり、あらためて設計判断が要る。
- UIの実装余地 - トップバーは既にSoft Bypass/Mono Preview追加で
  余裕がなく(ADR-008参照)、A/Bのための追加コントロール
  (A/B切替+コピー)を安全に配置するには、Phase 5後半の本格的な
  UI再設計(このADRの対象より広い)が必要になる。
- 試聴用途の機能であり、実際に価値を発揮するかは実DAWでの
  ワークフロー確認が要る(このコンテナでは検証できない)。

Undo/Redoが機能する今、複数のパラメータを変更してからCtrl+Zで
一括比較する、という部分的な代替は既に可能。A/B専用UIは
Phase 5の残りタスクとして`MANUAL_REQUIRED.md`ではなく通常の
未実装機能として引き続き扱う(設計自体が要検討のため、ADRの
「再検討条件」に記載)。

## 見送り: 多言語化(ローカライズ)

これは見送りではなく、**当初からの意図的な製品判断**である。
本プロジェクトのUIは日本語UIとして設計されている(3D UI導入時の
計画で「基本タブ」「日本語フォント」が明記され、多言語切替は
一度も採用方針に含まれていない)。全文字列を文字列テーブル化し、
言語切替UIを追加し、各言語でフォントフォールバック・折り返し・
ボタン幅を再検証する、という作業は、それ自体が独立した大きな
機能追加であり、ユーザーからの明示的な要求なしに独自に着手すべき
範囲を超えると判断した。英語(または他言語)UIが必要になった場合は、
独立したPhaseとして計画するのが適切。

## 実装: アクセシビリティ(軽量パス)

JUCEの標準コンポーネント(`Button`/`ToggleButton`/`ComboBox`/
`Slider`)は、追加のコードなしに、表示テキスト(ボタン文字列・
隣接するラベル)をアクセシビリティ名として公開するデフォルトの
`AccessibilityHandler`を持つ。本プロジェクトのコントロールは
既にすべて日本語の可視テキストを持つ(ボタン文字列、隣接ラベル、
ツールチップ)ため、追加コードなしで最低限のアクセシビリティ名は
既に提供されている。

これを超えるスクリーンリーダーでの実地確認(実際の読み上げ順序・
フォーカス移動・OS標準スクリーンリーダーとの組み合わせ)は、この
コンテナに実OS+支援技術の組み合わせがなく検証できないため、
`MANUAL_REQUIRED.md`に記載した。実装なしに「対応済み」と主張する
ことは避け、既定で得られる範囲と、確認が必要な範囲を明確に分けた。

## 理由

- Mono Preview/Undo/Redoは、既存のADR-008(Soft Bypass)と同じ設計
  パターン(最終段クロスフェード、標準APVTS機構の活用)を再利用でき、
  実装・検証コストが低い一方、実際のミキシング/校正ワークフローへの
  価値が明確。
- A/B・多言語化は、いずれもこのセッションの他の実装より一段大きい
  設計判断(永続化方式、UI再設計、言語切替アーキテクチャ)を要し、
  「実行していない検証を実行済みと書かない」という本パッケージの
  方針上、ここで拙速に実装するより、必要な設計判断を明示して次の
  Phaseへ回す方が誠実。

## 影響

### 良い影響

- Mono Preview: ミックスのモノ互換性チェックが、他のDAWツールを
  介さずプラグイン内で完結する。
- Undo/Redo: パラメータ操作・プリセット適用の誤操作から、ホストの
  プロジェクトUndoに頼らず即座に復帰できる。

### 悪い影響

- A/Bがないため、2つの設定を素早く切り替えて比較する、という
  ワークフローは今回サポートされない。

### 互換性

- 新規パラメータID `monoPreview` 追加。既存パラメータIDは変更なし。
- schemaバージョンの更新なし(既定false=従来動作のため不要)。
- `Presets::apply()`のシグネチャに既定値付きの新引数を追加(既存の
  呼び出し箇所は無改造で動作)。

## 実装

- file: `Source/dsp/HelixEngine.{h,cpp}`(`monoPreview`パラメータ、
  `monoPreviewSmoothed`、`process()`最終段のモノ折り畳み)、
  `Source/Parameters.h`(`monoPreviewID`)、
  `Source/PluginProcessor.{h,cpp}`(`undoManager`メンバ、
  `monoPreviewParam`)、`Source/PluginEditor.{h,cpp}`
  (トップバーのモノ確認トグル、`keyPressed()`によるUndo/Redo
  ショートカット)、`Source/Presets.h`(`undoManagerForOneStep`引数、
  全プリセットに`monoPreview=false`)

## テスト

- unit: `Tests/MonoPreviewTests.cpp`(既定falseがビット単位一致、
  ONで30ms以内に厳密にモノへ収束しOFFで復帰すること、Soft Bypassの
  後に適用されること、クロスフェードが滑らかであること)、
  `Tests/UndoRedoTests.cpp`(apvtsがundoManagerに正しく接続されて
  いること、単一のパラメータ変更のUndo/Redo、複数変更の単一
  トランザクションとしてのUndo/Redo、`Presets::apply`が
  プリセット名でトランザクションを開始すること)
- 全110件のCTest + ASan/UBSanがクリーン(0エラー、ASanビルドでのみ
  現れる`juce_Timer.cpp`のアサーション出力はAPVTS自身のタイマーに
  よる、本セッション以前からの既知の無害な現象で、テスト結果には
  影響しない)

## 再検討条件

A/B比較機能が実際に必要と判断された場合、まず永続化方式(セッション
限定 or プロジェクト保存対象)を決定してから、あらためてADRとして
設計する。多言語化は、英語(または他言語)UI提供が製品要件として
明示的に決定された時点で、独立したPhaseとして計画する。
