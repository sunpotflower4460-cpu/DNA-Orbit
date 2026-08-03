# DNA Orbit 意思決定ログ

## Decision ID

`ADR-007`

## 日付

2026-07-30

## 状態

- Accepted

## 題名

Host Phase Lock(Free/Retrigger/Host Lock)を、比例制御による毎サンプル補正で実装する

## 背景

仕様書は、ホストのトランスポート位置(PPQ)と軌道の位相を一致させる
モードを要求している。用途は主に「ループ再生のたびに軌道の見た目/
音が毎回同じ位置から始まる」「オフラインバウンスでも再生位置に対して
決定論的な位相になる」こと。3つのモードを実装した:

- **Free**(既定・index 0): 従来どおり、Rateから積分される連続位相。
  PPQには一切依存しない。
- **Retrigger**(index 1): ホストの再生が停止→開始した瞬間だけ、
  θAをStart Phase/Directionから決まる角度へスナップする
  (対称性ロックも強制的に再有効化する)。
- **Host Lock**(index 2): 再生中は常に、θAをホストのPPQ位置から
  導かれる目標角度へ追従させ続ける。

## 問題: 目標角度への追従方法

Host Lockの目標角度は
`target = 2π · frac(direction · (ppq - startPhaseBeats) / cycleBeats)`
で毎ブロック計算できる(仕様書の式そのもの)。しかし、この目標値を
そのままθAへ**スナップ**すると、PPQ報告の通常のジッタや丸め誤差でも
毎ブロッククリックノイズが出る。逆に、既存のSymmetry再同期
(`resyncSamplesRemaining` を使った有界ランプ)と同じ設計を流用すると、
新たな状態遷移ロジック(ランプ開始条件・残りサンプル数の管理)が
Retrigger/通常ドリフト/トランスポートジャンプの3ケースそれぞれで
必要になり、複雑化する。

## 決定

**比例制御(P制御)による毎サンプル角速度の補正項**を採用した:

```
error = shortestAngleDelta(thetaA, targetTheta)   // 毎ブロック1回計算
correction = error / (hostLockCorrectionTimeConstantSeconds * sampleRate)
incA = angularIncrement(rateHz, sampleRate) + correction   // 毎サンプル加算
```

`hostLockCorrectionTimeConstantSeconds = 0.03`(30ms)。これは仕様書が
要求する20-50msの収束窓に収まる一次系(指数収束、時定数30ms)になる。

Retrigger単独の「開始した瞬間に一度だけスナップする」動作は、この
補正とは別に、ブロック先頭で `currentHostIsPlaying && ! wasHostPlaying`
を検出した際にθA/θBを直接書き換える、単純な条件分岐で実装した
(状態はplay/stopのエッジ検出用の`wasHostPlaying`一つのみ)。

## 比例制御を選んだ理由

- **状態が要らない**: 既存のSymmetry再同期のような
  「残りランプサンプル数」を持たない。目標角度を毎ブロック再計算し、
  現在の誤差からその場で補正量を出すだけなので、トランスポートジャンプ
  (ループ・スクラブ)後も次のブロックで自動的に正しく追従を再開する。
  ランプの「進行中に新しいジャンプが来たらどうするか」という
  Symmetry再同期側で必要だった特別扱いが、Host Lockでは発生しない。
- **安定性**: 一次系なのでオーバーシュートしない。誤差が大きい
  (トランスポートジャンプ直後)ほど補正が強くかかり、収束するほど
  補正が弱まる。発振の可能性がない。
- **通常ドリフトと大ジャンプを同じ式で扱える**: 毎ブロックの
  微小な浮動小数点誤差の是正も、ループ地点での大きな飛びも、
  同じ`error/timeConstant`の式が両方をカバーする。特別扱いの分岐が
  ケースの数だけ増えない。

## 拍子(Time Signature)対応

`cycleBeats`(1周が何拍か)は、旧`divisionIndexToBeats(index)`
(4/4前提の固定値)を`divisionIndexToBeats(index, quarterNotesPerBar)`
へ拡張し、ホストの`TimeSignature`(`numerator`/`denominator`)から
`quarterNotesPerBar = numerator * 4 / denominator`を計算して渡す
ことで、3/4や6/8といった拍子でも小節境界が一致するようにした。
4/4では旧関数と全ケースでビット同一(検証済み)なので、この拡張は
既存の`syncedRateHz`の呼び出し元(Sync ON時のRate計算)にも安全に
反映される。

## 実装しなかったこと: 「Sync ON時はHost Lockを既定にする」

仕様書は「テンポ同期がONのときはHost Lockを既定にする」ことを
示唆しているが、これは実装しなかった。理由:
- `phaseMode`の既定値は仕様非依存に固定(常にFree)しておく方が、
  「ある1つのパラメータの値が、別のパラメータの状態によって暗黙に
  変わる」という反応的なデフォルトのロジックを避けられ、状態遷移が
  単純になる(schemaバージョンやプリセット互換性の判断も単純化する)。
- ユーザーは`phaseMode`を明示的にHost Lockへ切り替えることで、
  いつでも同じ効果を得られる。既存プロジェクト・既存プリセットの
  挙動を変えない(下記「schemaバージョンの更新」参照)ことを優先した。

## schemaバージョンの更新は不要

`phaseMode`の既定値Free(index 0)は、この機能が存在する前の
エンジンの挙動(Rateから積分される連続位相、PPQ非依存)と完全に
一致する。`startPhaseDeg`の既定0°・`clockwise`の既定true(CW)も、
Free/Retrigger以外のモードが選ばれない限り出力に一切影響しない。
したがって、Character(ADR-006)・Bass Anchor(ADR-005)と同じ
判断基準により、**schemaバージョンの更新も移行ロジックも不要**
(`Tests/BaselineRegressionTests.cpp`のFree/既定値でのビット一致に
より検証)。

## 理由

- 音質・機能: Host Lockはループ・オフラインバウンスでも決定論的な
  位相を実現する。Retriggerは「再生開始のたびに同じ見た目/音から
  始まる」という、より軽量な代替を提供する。
- 実装難度: 比例制御は追加の永続状態(`wasHostPlaying`のみ)で済み、
  既存の毎サンプルループへの変更は`incA`への1行加算のみ。
- テスト可能性: 目標角度・収束時間・方向反転・トランスポートジャンプ
  時の滑らかさ・決定論性のすべてが、独立した閉形式の期待値で
  検証できる。

## 影響

### 良い影響

- ループ再生・オフラインバウンスで、DAWのタイムラインと軌道位相が
  一致する新しい選択肢が加わる。
- 既存プロジェクトは(Free既定のため)一切影響を受けない。

### 悪い影響

- Host Lockの30ms時定数・比例制御という設計は、実DAWでの
  ループ地点・テンポ変更・スクラブといった実運用下での聴感上の
  自然さを、この環境では試聴できない(`MANUAL_REQUIRED.md`参照)。

### 互換性

- 新規パラメータID `phaseMode` / `startPhase` / `direction` 追加。
  既存パラメータIDは変更なし。
- schemaバージョンの更新なし(Free/0°/CW=既定=従来動作のため不要)。

## 実装

- file: `Source/dsp/HelixEngine.{h,cpp}`(`phaseMode`/`startPhaseDeg`/
  `clockwise`/`hostCycleBeats`/`hostPpqPosition`/`hostIsPlaying`
  パラメータ、ブロック単位のRetrigger/Host Lock処理、毎サンプル
  `hostLockCorrectionPerSample`の加算)、`Source/Parameters.h`
  (`phaseModeID`/`startPhaseID`/`directionID`、
  `divisionIndexToBeats`の拍子対応拡張)、`Source/PluginProcessor.cpp`
  (`getPlayHead()`からのPPQ/再生状態/拍子の取得)、
  `Source/PluginEditor.{h,cpp}`(詳細タブへ位相モード/方向の
  コンボボックスを追加。Start Phase専用のノブはPhase 5のUI刷新まで
  意図的に見送り、ホストの汎用パラメータリスト経由で操作可能)、
  `Source/Presets.h`(全プリセットにFree/0°/CWを設定)

## テスト

- unit: `Tests/HostPhaseLockTests.cpp`
  (Retriggerが再生開始の瞬間だけStart Phaseへリセットすること、
  Host LockがPPQ由来の目標角度へ定常誤差なく収束・追従すること、
  Direction(CCW)で追従方向が反転すること、大きなトランスポート
  ジャンプ後もサンプル間跳躍が滑らかであること、同一のPPQ/
  パラメータ列に対して決定論的であること)
- 全97件のCTest + ASan/UBSanがクリーン(0エラー)

## 再検討条件

実DAWでのループ・テンポ変更・スクラブの試聴で、30ms時定数の
収束が硬すぎる/緩すぎると判明した場合、または「Sync ON時に
Host Lockを既定にする」挙動が実際に強く求められると判明した場合に
見直す。
