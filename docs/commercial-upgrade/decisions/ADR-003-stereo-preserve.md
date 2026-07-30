# DNA Orbit 意思決定ログ

## Decision ID

`ADR-003`

## 日付

2026-07-30

## 状態

- Superseded by ADR-004

**2026-07-30 追記**: 本ADRが採用した`sourceA = M + p*S` / `sourceB = M - p*S`
方式(両ストランドへ直接L/R寄りの信号を送る)は、Phase 2.5の再検証で
「幾何学的に対蹠(position)を保っていても、エネルギー加重中心が偏りうる」
という不備が判明したため、`ADR-004-stereo-preserve-bed.md`の方式に
置き換えられた。以下の本文は歴史的記録として残す。

## 題名

Stereo Preserve (M/S ソース分離) を実装し、仕様書のエネルギー補正normalizerは今回見送る

## 背景

コードレビューで指摘されたP0: Wetの各strandは`wetSource = 0.5*(L+R)`という
共有Mid downmixから作られており、Side情報が失われていた。`L ≈ -R`の入力で
Wetがほぼ無音になる、広いステレオ素材がMixを上げるほど中央化する、といった
実害があった。`02_DSP再設計仕様書.md` §3はこれを`stereoPreserve`パラメータ
(M/Sベースのsource分離)で解決するよう指示している。

## 選択肢

### A: 何もしない

P0を放置。

### B: `stereoPreserve`によるsource分離のみ実装し、仕様書§3.3のエネルギー
補正normalizerは見送る(採用)

### C: 仕様書§3.3のnormalizerを文言通りに実装する

```text
normalizer = 1 / sqrt(max(0.5, 0.5 * (1 + p^2)))
```

を検証すると、`p=0`で`normalizer = sqrt(2) ≈ +3dB`、`p=1`で`normalizer = 1
(0dB)`になる。すなわちこの式は**p=0で最大の補正(+3dB)**を要求しており、
これは「schema 1(`stereoPreserve=0`のレガシー再現)は既存の
`Tests/BaselineRegressionTests.cpp`のフィンガープリントと数値的に一致
しなければならない」という、本プロジェクトが最初から堅持している
不変条件と直接衝突する。文言通りに実装すると、既存プロジェクトを読み込んだ
瞬間に+3dBの音量変化が発生してしまう。

### D: 仕様書の式を`p=0`で0dBになるよう相対化する

```text
normalizer' = normalizer(p) / normalizer(0) = 1 / sqrt(1 + p^2)
```

`p=0`で0dB(何もしない)、`p=1`で `-3dB`。意図(Stereo Preserveを動かしても
Wetの体感音量を一定に保つ)は残しつつ、レガシー互換の不変条件と両立する。
ただし実測(パンダ/パッド/ボーカル等の実素材)で検証していない式であり、
今回は導入を見送る。

## 決定

**B**。P0(anti-phase入力でWetが消える)の実害は、`stereoPreserve`の
既定値を70%にすることで解消される。エネルギー補正(仕様書§3.3)は
「Stereo Preserveつまみ自体の体感音量を一定に保つ」という副次的な
洗練であり、DSP完了条件(§14)の必須項目には含まれていない。
Cは既存の不変条件と衝突するため却下。Dは筋が良いが実測なしに数値を
確定させるべきでないため、今回は見送り、将来ADR-004として実測の上で
再提案する対象として明記する。

なお、既存のGeometry Wet Compensation(既存のWetMakeupゲイン予測式、
`HelixEngine::process`内の`makeupTarget`)は`stereoPreserve`を一切
参照しないため変更なし。これは信号非依存・幾何のみに基づく式であり、
`stereoPreserve`によって精度が多少劣化する可能性はある(strand A/Bが
同一信号のコピーでなくなるため、delayベースのcoherence推定が理想的では
なくなる)が、既存のクランプ(±12dB上限)により有界であることに変わりはなく、
挙動が不安定になることはない。

## 理由

- 音質: P0の実害はsource分離だけで解消する。エネルギー補正は「なくても
  壊れない」洗練であり、優先度が低い。
- 互換性: schema 1 (`stereoPreserve=0`)が既存のBaselineRegressionTests
  フィンガープリントと数値的に一致するという、本プロジェクトの
  最初からの不変条件を一切変えない。
- 実装難度・テスト可能性: Bは既存のWet Makeup式・Mix Law・NULL CORE
  ロジックに一切手を入れずに済み、影響範囲が狭い。Cは前述の通り不変条件と
  衝突し、Dは実測データなしに数値を確定させることになり、いずれも
  「実測していないことを実装済みと書かない」という原則に反する。

## 影響

### 良い影響

- anti-phaseステレオ入力でWetが消えなくなる(新規インスタンス既定70%)。
- Coreもステレオ幅を保持するようになる(`sourceA`/`sourceB`をそのまま
  Core信号として再利用: `coreL = lerp(M, L, p) == sourceA`)。
- schema 1プロジェクトは一切音が変わらない(`stereoPreserve`を0に強制)。

### 悪い影響

- Stereo Preserveつまみを動かすと(既存のGeometry Auto Gainとは別に)
  体感音量がわずかに変化しうる。実測で問題になれば選択肢Dを再検討する。

### 互換性

- 新規パラメータID`stereoPreserve`追加。既存パラメータIDは変更なし。
- state schema version 2に更新。schema 1の保存データは
  `stereoPreserve`ノードが存在しないため、ロード時に0%へ強制される
  (`PluginProcessor::setStateInformation`)。

## 実装

- file: `Source/dsp/HelixEngine.{h,cpp}` (`stereoPreserve01`, M/S分離,
  Core再利用), `Source/Parameters.h` (`stereoPreserveID`,
  `currentStateSchemaVersion`を2へ, `legacyUnversionedSchema`定数の追加),
  `Source/PluginProcessor.cpp` (schema<2でのstereoPreserve強制0),
  `Source/PluginEditor.{h,cpp}` (詳細タブへノブ追加),
  `Source/Presets.h` (全プリセットに70%を設定)
- migration: `PluginProcessor::setStateInformation`が起動時に一度実行

## テスト

- unit: `Tests/StereoPreserveTests.cpp`(0%でレガシー無音維持、
  70%で可聴、anti-phaseでの線形スケーリング、モノ入力は無影響、
  100%でL/R由来が保たれる、パラメータ自動化のスムージング)、
  `Tests/BaselineRegressionTests.cpp`(schema 1の数値フィンガープリント、
  `stereoPreserve01=0.0f`を明示指定して無改造で通過)、
  `Tests/StateTests.cpp`(新規インスタンス70%、schema 1ロードで0%強制、
  schema 2の任意値がラウンドトリップする)
- 全71件のCTest + ASan/UBSanがクリーン(0エラー)
- manual: 実際のステレオ素材(ボーカル・パッド・ワイドギター等)での
  試聴は`MANUAL_REQUIRED.md`に記載。特にStereo Preserve 70%が
  「centerが0になる」体験として自然に聞こえるかは実DAWでの確認が必要。

## 再検討条件

実DAWでの試聴でStereo Preserveつまみの体感音量変化が問題になった場合、
選択肢Dの相対化normalizer(`1/sqrt(1+p^2)`)をADR-004として実測の上で
再提案する。
