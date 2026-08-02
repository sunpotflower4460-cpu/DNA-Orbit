# DNA Orbit UI/UX再設計仕様書

## 1. UIの役割

DNA OrbitのUIは「宇宙的で美しい画面」だけでは不足する。

UIの役割は次の三つ。

1. 音がどう動いているかを即座に理解させる
2. 危険な位相状態をユーザーへ正直に伝える
3. 複雑なDSPを、音楽的な言葉で操作可能にする

---

# 2. 情報設計

## 2.1 Header

左:

- DNA ORBIT
- Flower Pot Studio
- 小さなversion表示

中央:

- Preset selector
- Favorite
- Previous / Next

右:

- A/B
- Undo
- Redo
- Soft Bypass
- Settings

## 2.2 Main Visual

中央の3D DNAを最大の主役にする。

表示:

- Strand A
- Strand B
- Center axis
- Centroid trail
- Front / Backの奥行き
- 回転方向
- 現在位相
- Low anchor zone
- 相関状態
- Host Lock状態

### 直接操作

- 横ドラッグ: Width
- 縦ドラッグ: Depth
- Shift+横: 微調整
- Alt/Option+ドラッグ: Start Phase
- スクロール: Motion
- 中心軸ドラッグ: Core
- ダブルクリック: 該当値リセット

直接操作はDetailのノブと双方向同期する。

## 2.3 Basic

推奨5ノブ:

1. Motion
2. Width
3. Depth
4. Core
5. Mix

日本語:

1. 動き
2. 広がり
3. 奥行き
4. 中心
5. 効果量

補助テキストは短くする。

例:

- 動き: 1周の速さ
- 広がり: 左右の幅
- 奥行き: 前後の距離
- 中心: 真ん中の芯
- 効果量: 原音との比率

## 2.4 Detail

セクション分けする。

### Motion

- Sync
- Division
- Phase Mode
- Start Phase
- Direction
- Symmetry

### Space

- Stereo Preserve
- Twist
- Bass Anchor
- Character

### Mix

- Auto Level
- Output
- NULL CORE
- Hollow Safety

1画面に入りきらない場合はスクロールではなく、折りたたみかサブタブを使う。

---

# 3. パラメータ状態の見せ方

## 3.1 Sync ON

- Motionノブを無効化しない場合は表示値をDivisionへ切り替える
- `SYNC`バッジ
- Host tempo unavailable時は小さく`FREE FALLBACK`
- Rateが音に効いていない状態を隠さない

## 3.2 NULL CORE ON

- Coreノブをdim
- Coreの下に「NULL CORE中は無効」
- Mono Previewを自動表示
- 警告アイコン + テキスト
- 相関が負へ近づくほど警告を強める
- 赤だけでなく形と文言でも示す

## 3.3 Bass Anchor

DNA表示の中心軸下部または周囲へ、低域が固定されることを穏やかな帯で示す。

ただし周波数スペクトラムのような誤解を招く表示は避ける。

## 3.4 Host Lock

- `LOCKED TO SONG`
- `FREE`
- `RETRIGGER`

を小さな状態チップで表示。

---

# 4. Preset UX

## 4.1 プリセットは全パラメータを定義

現在のように一部だけ変更して以前の状態を引き継がない。

必ず定義:

- Sync
- Division
- Rate
- Width
- Depth
- Symmetry
- Twist
- Core
- Stereo Preserve
- Bass Anchor
- Character
- Mix
- Auto Level
- Output
- NULL CORE
- Hollow Safety
- Phase Mode
- Start Phase
- Direction

## 4.2 カテゴリ

- Vocal
- Instrument
- Synth
- Ambient
- Rhythmic
- Experimental
- Utility

## 4.3 初期プリセット候補

### Vocal

- Vocal Halo
- Wide but Centered
- Breath Orbit
- Chorus Lift

### Instrument

- Acoustic Motion
- Electric Drift
- Piano Halo
- String Spiral

### Synth

- Slow Helix
- Neon Rotation
- Pulse Orbit
- Deep Spiral

### Utility

- Mono Safe Width
- Bass Anchored
- Subtle Movement
- A/B Calibration

### Experimental

- Hollow Core
- Unstable Genome
- Twin Eclipse
- Centroid Wander

## 4.4 Modified表示

プリセット選択後に値を変えたら:

- `Preset Name *`
- Save As
- Revert

## 4.5 A/B

A/Bは全音響パラメータを保持する。

- AからBへコピー
- BからAへコピー
- 比較時に音量差が過度に出ない
- stateへ保存

---

# 5. 視覚デザイン

## 5.1 方向性

- dark navy
- cyan strand
- emerald strand
- white center
- orange drift
- red danger

既存配色は維持可能。

## 5.2 世界観

「宇宙船の計器」ではなく、

> 生命科学と宇宙観測が融合した、静かな未来の楽器

を目標とする。

避ける:

- 過剰なネオン
- 小さすぎる文字
- 無意味なグロー
- 何でも点滅
- ゲームUI化
- 重要度のない数値の大量表示

## 5.3 数値表示

現在の技術的readoutはDetailまたはInfo overlayへ移す。

Basic常時表示は最小限:

- cycle
- correlation
- mode

専門値:

- A pan
- B pan
- centroid
- phase error

はDetailで表示。

## 5.4 3D描画

既存のJUCE 2D APIによる3D表現を維持。

条件:

- Editor非表示時に停止
- Adaptive quality
- Paint内allocation抑制
- 24fpsでも意味が伝わる
- 画面最大化でCPUが跳ねない
- Retina / HiDPIでぼやけない

---

# 6. レスポンシブレイアウト

## 最小 780×540

- Header
- Main visual
- Basic controls
- Detailは簡略配置

## 標準 1000×680前後

- Main visualを広く
- readoutは右側
- controlsは下部

## 大型 1600×1100

- 無意味にノブを巨大化しない
- 余白とvisualを拡大
- Detailを2カラム化
- 描画品質はCPU予算内で調整

レイアウトは固定pxの寄せ集めではなく、割合と最小幅で設計する。

---

# 7. 操作性

すべてのノブ:

- ダブルクリックでDefault
- Shiftで微調整
- 数値入力
- 右クリックでHost context menu
- Automation gestureを正しく開始/終了
- Tooltip
- Tabキー移動
- VoiceOver/Narrator用名称

ComboBox:

- キーボード選択
- 読み上げ
- 現在値が切れない

Button:

- ON/OFFを色だけにしない
- focus ring
- disabled理由をtooltip表示

---

# 8. ローカライズ

言語:

- Auto
- 日本語
- English

新規ID候補:

- `language`は音響パラメータではなくeditor stateでもよい

文字列をコード内に散在させず、キー管理する。

例:

```text
basic.motion.name
basic.motion.hint
detail.stereoPreserve.tooltip
warning.nullCoreMono
status.hostLocked
```

フォント:

- macOS: Hiragino Sans
- Windows: Yu Gothic UI
- Linux: Noto Sans CJK JP
- fallback検証

---

# 9. アクセシビリティ

- コントラストWCAG AA相当を目安
- 12px未満の重要文字を避ける
- 赤/緑だけで状態を区別しない
- motion reduction設定
- animation intensity設定
- スクリーンリーダー名
- keyboard-onlyで全操作
- focus順を論理的にする

Settings:

- Language
- Animation: Full / Reduced / Off
- Visual quality: Auto / High / Low
- Tooltips: On / Off

---

# 10. UI状態保存

保存する:

- window width/height
- page
- language
- animation preference
- visual quality preference
- A/B state
- selected preset metadata

音響stateとeditor-only stateを分ける。

---

# 11. UI完了条件

- 780×540〜1600×1100で重なり・切れなし
- 100/125/150/200% DPIで破綻なし
- 日本語・英語の文字化けなし
- Sync中にRateの意味が明確
- NULL CORE中にCoreが無効と分かる
- Basicだけで実用音へ到達できる
- Detailで全パラメータへアクセス可能
- keyboard-only操作可能
- Editorを閉じるとtimer停止
- 複数インスタンスでUI CPUが許容範囲
- host automationと画面表示が常に一致
