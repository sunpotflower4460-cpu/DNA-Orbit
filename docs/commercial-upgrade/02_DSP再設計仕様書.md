# DNA Orbit DSP再設計仕様書

## 1. 現在の主な課題

### 1.1 Wet側のステレオ情報消失

現在のWet sourceは概ね次である。

```cpp
wetSource = stereoIn ? 0.5f * (L + R) : L;
```

このためSide情報が失われる。

重大ケース:

- `L = -R`に近い入力でWetが消える
- 広いステレオ素材がMixを上げるほど中央化する
- 非相関ステレオがレベル低下する
- 元のステレオ配置とDNA運動の関係が切れる

### 1.2 Auto GainとMix Law

現在はWet単体の予測ゲインをDryへ近づけ、その後Equal Power Mixを行う。

DryとWetの相関が高い場合、50%付近で加算によるレベル上昇が起こり得る。  
したがって「Wet 100%のレベルが近い」ことと「Mix全域で体感音量が一定」は別問題である。

### 1.3 ホストバイパス中の内部状態停止

`processBlockBypassed()`がEngineを進めないため、復帰時に古いDelay内容・古い軌道位相・フィルター状態が現れる可能性がある。

### 1.4 テンポ同期の再現性

現在はBPMからRateを求める速度同期であり、PPQ位置から位相を求めない。

- 再生開始位置で結果が変わる
- オフラインバウンスとリアルタイムの開始位相が揃わない可能性
- ループ時に軌道位相が意図せず継続する
- 4/4以外のbars換算が正しくない

### 1.5 CPU負荷

サンプル単位で以下が発生している。

- `sin/cos`
- `fmod`
- LPF係数用`exp`
- coherence用`exp`
- Dry/Wet用`sin/cos`

192kHz・複数インスタンス時の測定が必要。

---

# 2. 新DSP信号フロー

```text
Input L/R
  │
  ├─ Input sanitize
  │
  ├─ M/S decomposition
  │
  ├─ Bass Anchor crossover
  │      ├─ Low band → stable/core path
  │      └─ High band → orbit source
  │
  ├─ Stereo Preserve mapping
  │      ├─ Strand A source
  │      └─ Strand B source
  │
  ├─ Orbit phase generator
  │      ├─ Free
  │      ├─ Retrigger
  │      └─ Host Lock
  │
  ├─ Strand A processing
  │      ├─ front/back gain
  │      ├─ spectral cue
  │      ├─ fractional delay
  │      └─ pan
  │
  ├─ Strand B processing
  │      ├─ front/back gain
  │      ├─ spectral cue
  │      ├─ fractional delay + Twist
  │      └─ pan
  │
  ├─ Core path
  ├─ Low band reinjection
  ├─ NULL CORE / Hollow processing
  ├─ Wet normalization
  ├─ Correlation-aware Dry/Wet
  ├─ Output trim
  └─ Meter / visual state
```

---

# 3. ステレオ保持モデル

## 3.1 M/S

```text
M = 0.5 × (L + R)
S = 0.5 × (L - R)
```

## 3.2 Stereo Preserve

新規パラメータ:

- ID: `stereoPreserve`
- version: 1
- Range: 0〜100%
- Default: 70%
- Basicでは非表示
- Presetごとに値を設定

```text
p = stereoPreserve / 100

sourceA = M + p × S
sourceB = M - p × S
```

`p=0`:

- A/BともM
- 旧挙動に近い

`p=1`:

- A=L
- B=R

## 3.3 エネルギー補正

M/S合成後のエネルギー差を極端に補正しない。

推奨:

```text
normalizer = 1 / sqrt(max(0.5, 0.5 × (1 + p²)))
```

ただし最終式はテスト信号で評価し、補正上限を±3dB程度に制限する。

## 3.4 Core

Coreはモノ信号をL/Rへ複製するだけではなく、Stereo Preserveに応じて元の像を保持する。

推奨:

```text
coreL = lerp(M, L, coreStereo)
coreR = lerp(M, R, coreStereo)
```

新規パラメータを増やしすぎないため、`coreStereo = stereoPreserve`を第一候補とする。

## 3.5 互換モード

旧プロジェクトの音を保つため、stateにDSP schema versionを保存する。

- schema 1: 現行Mid由来
- schema 2: Stereo Preserve対応

既存stateを読み込んだ場合:

- `stereoPreserve = 0`
- 既存音を再現

新規インスタンス:

- `stereoPreserve = 70`

製品判断として音の改善を優先する場合でも、少なくとも移行ルールを文書化する。

---

# 4. Bass Anchor

## 4.1 目的

- キックとベースの中心を安定
- モノ互換改善
- 低域の位相揺れ削減
- 高いDepthでもミックスで使いやすくする

## 4.2 パラメータ

- ID: `bassAnchorHz`
- Range: 20〜500Hz
- Default: 120Hz
- 20Hz付近を`Off`表示してもよい
- Skew: 低域を細かく操作できる対数寄り

## 4.3 クロスオーバー

推奨:

- 4th-order Linkwitz–Riley
- 低域と高域の再合成でフラット
- 係数更新はControl Rate
- cutoff変更は平滑化
- filter stateは左右別

低域:

- Coreへ残す
- Output上で元のL/RまたはM/Sを保持
- NULL CORE時もSafety設定に応じて残す

高域:

- DNA軌道へ送る

## 4.4 テスト

- 20Hz〜20kHz sweepで再合成誤差
- cutoff自動化時のクリック
- 低域のL/R相関
- Mix 0/100での位相
- 複数サンプルレート

---

# 5. Orbit Phase Generator

## 5.1 Phase Mode

新規choice:

- ID: `phaseMode`
- Choices:
  1. Free
  2. Retrigger
  3. Host Lock
- Default: Host Lock when Sync ON, Free when Sync OFF

### Free

現在に近い連続位相。

### Retrigger

ホストが停止→再生へ変化したとき、`startPhase`へ戻す。

### Host Lock

PPQ位置から位相を直接計算する。

```text
cycleBeats = divisionから算出
phase = 2π × fract((ppqPosition - phaseOffsetBeats) / cycleBeats)
```

## 5.2 Start Phase

- ID: `startPhase`
- Range: 0〜360°
- Default: 0°
- UI上は視覚ポイントをドラッグ可能

## 5.3 Direction

- ID: `direction`
- Choice: CW / CCW
- Default: CW

## 5.4 Time Signature

bars表記はホストの分子・分母を考慮する。

少なくとも:

```text
quarterNotesPerBar = numerator × 4 / denominator
```

ホスト情報がない場合:

- 4/4 fallback
- UIへfallback状態を出す必要はない
- debug logまたはテストで追えるようにする

## 5.5 Transport Jump

PPQが不連続に変化した場合:

- Host Lock: 新位相へ短い20〜50msの位置クロスフェード
- Retrigger: 再生開始時のみリセット
- Free: そのまま継続

オフラインレンダーでは常に決定論的であること。

---

# 6. Strand Processing

## 6.1 前後ゲイン

現在の最大4dB減衰を基準に、Characterで変える。

- Natural: 最大3dB
- Vivid: 最大5dB
- Deep: 最大7dB

## 6.2 スペクトルキュー

単純な一段LPFのみでは音色変化が強く、不自然になりやすい。

推奨順:

1. 高域シェルフ
2. 緩いLPF
3. 必要なら中高域のわずかなPresence

Natural例:

- back high shelf: -2.5dB @ 5kHz
- back LPF: 12〜16kHz
- front presence: +0.5dB @ 3kHz

Vivid例:

- back high shelf: -4dB
- back LPF: 8〜12kHz

Deep例:

- back high shelf: -5dB
- back LPF: 5〜9kHz
- early reflectionを少量

数値は固定決定ではなく、試聴と測定で調整する。

## 6.3 Delay

- Back delay最大: Natural 3ms / Vivid 6ms / Deep 9ms
- Twist: 0〜20ms
- 高速Rate時のdelay modulationによるピッチ変調を測定
- 必要ならDelay変化をさらに平滑化
- 速い変化時は最大Delay量を自動制限する選択肢を検討

## 6.4 Character

新規choice:

- ID: `character`
- Natural
- Vivid
- Deep
- Default: Natural

Basicではプリセット内部設定。Detailで表示。

---

# 7. Symmetry

現状の「BのRate差で中心が漂う」思想は維持する。

改善:

- 100%付近の感度を緩やかにする
- 95〜100%で急に不安定にならないカーブ
- drift量をRateに対して知覚的に一定化
- Host Lock時のB位相関係を明示する

推奨マッピング:

```text
driftFactor = pow(1 - symmetry, 2) × maxDifference
```

これにより高Symmetry域を微調整しやすくする。

既存音互換が必要なら旧カーブをschema 1で維持。

---

# 8. NULL CORE / Hollow Core

## 8.1 現在の定義

WetのMidを0にし、Side onlyへする。

## 8.2 改善

新規Safety:

- ID: `hollowSafety`
- Range: 0〜30%
- Default: 10%
- 0%で完全Side only
- 10%以上で少量Midを残す

```text
wetMidOut = wetMid × safety
wetSideOut = wetSide
```

完全NULL COREプリセットのみ0%。

## 8.3 UI連動

- NULL CORE中はCoreノブを無効表示
- Mono Previewを目立たせる
- Correlationが危険域なら警告
- 警告は色だけに依存しない
- tooltipに「Wetがモノで消える可能性」を明記

---

# 9. Auto LevelとMix Law

## 9.1 目標

- Mix 0〜100%で大きな体感音量変化がない
- ポンピングしない
- NULL COREで無限補正しない
- 信号依存補正は遅く、上限付き
- オフラインとリアルタイムで一致

## 9.2 推奨二段構造

### A. Geometry Wet Compensation

既存の決定論的Wet予測を改善して維持。

- Stereo Preserveを計算へ含める
- Bass Anchorを含める
- Characterのゲインを含める
- 補正上限: ±6〜9dB
- NULL CORE完全時は補正を抑制

### B. Correlation-aware Mix Law

Dry/Wet相関`rho`を低速推定。

候補式:

```text
gD = cos(m × π/2)
gW = sin(m × π/2)

predictedPower = gD² + gW² + 2 × rho × gD × gW
normalizer = 1 / sqrt(max(epsilon, predictedPower))
```

出力:

```text
out = normalizer × (gD × dry + gW × wet)
```

制約:

- `rho`は-1〜1
- attack 100〜300ms
- release 500〜1500ms
- normalizer範囲を例: -3〜+3dB
- silence時は1へ戻す
- NULL CORE時の逆相に注意
- Auto Level OFFでは補正なし

第一実装が不安定なら、信号依存ではなくプリセット/幾何ベースの相関推定から開始する。

## 9.3 テスト

- Mixを0→100→0へ連続スイープ
- mono tone
- pink noise mono
- wide stereo
- anti-phase stereo
- transient
- silence
- NULL CORE
- Core 100
- Radius 0
- Twist 0
- 最大Depth

判定:

- 通常素材の長期RMS偏差: ±1.5dB目標
- 瞬間ピーク: クリップしない
- ポンピング: 手動試聴で不自然さなし
- オートメーション時クリックなし

---

# 10. Bypass

## 10.1 Host Bypass

ホストバイパス中も内部状態を進める。

手順:

1. 入力を事前確保したdry bufferへ保存
2. Engineへ入力して状態更新
3. 出力へdry bufferを復元

禁止:

- processBlockBypassed内でメモリ確保
- 毎回AudioBufferを新規生成

## 10.2 Soft Bypass

新規パラメータ:

- ID: `softBypass`
- bool
- Default: false

50〜100msでDryへクロスフェードし、内部状態は継続。

## 10.3 テスト

- 10秒バイパス後復帰
- feedbackのような古い残響が出ない
- 軌道位置が意図通り
- automationで連続ON/OFF
- block size 16
- 192kHz

---

# 11. CPU最適化

## 11.1 Control Rate

次を8〜16サンプル単位で計算し補間する。

- filter coefficients
- coherence estimate
- Auto Level target
- Character parameter mapping

## 11.2 Trigonometry

- `sin`と`cos`は一度ずつ計算
- 対称ロック時:
  - `sin(theta + π) = -sin(theta)`
  - `cos(theta + π) = -cos(theta)`
- Mix gainはパラメータ変更時またはControl Rate更新
- `fmod`は位相が範囲超過した時だけ実行

## 11.3 ベンチマーク目標

基準環境を記録して相対評価する。

目安:

- Editor closed、48kHz/64 samples、1 instance: 0.5%未満を目標
- Editor open: DSPとの差分1%未満を目標
- 10 instancesで安定
- 192kHzでもドロップアウトなし
- Audio callback内allocation 0

絶対数値は実機依存のため、CIでは前版比の回帰検出を重視する。

---

# 12. Double Precision

`supportsDoublePrecisionProcessing()`を導入する場合:

- float/double共通template Engineを検討
- 過度なコード複製を避ける
- stateとvisualはfloatでも可
- float/double出力差のテスト
- 実装負担がP0を遅らせるならP2へ回す

---

# 13. パラメータ一覧

既存IDは変更禁止。

新規候補:

| ID | 種類 | 既定 |
|---|---|---:|
| `stereoPreserve` | Float 0-100% | 70 |
| `bassAnchorHz` | Float 20-500Hz | 120 |
| `phaseMode` | Choice | Host Lock |
| `startPhase` | Float 0-360° | 0 |
| `direction` | Choice | CW |
| `character` | Choice | Natural |
| `hollowSafety` | Float 0-30% | 10 |
| `softBypass` | Bool | Off |
| `language` | Choice | Auto |

パラメータ数を増やしすぎないため、試作後に統合可能性を見直す。

---

# 14. DSP完了条件

- anti-phase stereoでもHELIX Wetが無音にならない
- Stereo Preserve 0で旧挙動に近い
- Stereo Preserve 100でL/R由来が保たれる
- Bass Anchor再合成がフラット
- Host Lockで同一位置から同一出力
- bypass復帰で古い音が出ない
- Mix全域の音量変動が制御される
- parameter automationでクリックなし
- sample rate 44.1〜192kHzでfinite
- block size 1または最小対応値から4096までfinite
- processBlock内allocation 0
- CTest・pluginval・VST3 Validator通過
