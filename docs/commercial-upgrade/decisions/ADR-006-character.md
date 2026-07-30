# DNA Orbit 意思決定ログ

## Decision ID

`ADR-006`

## 日付

2026-07-30

## 状態

- Accepted

## 題名

Character(Natural/Vivid/Deep)を、仕様書の絶対数値ではなく既存の固定定数を"Natural"として実装する

## 背景

仕様書§6は、背後(back)位置での減衰・スペクトルキュー・ディレイの強さを
Natural/Vivid/Deepの3段階に分けることを提案し、各モードの絶対数値
(Natural: 最大3dB減衰・back LPF 12-16kHz・back delay 3ms、Vivid: 5dB・
8-12kHz・6ms、Deep: 7dB・5-9kHz・9ms)を例示している。

## 問題

このプロジェクトの既存実装は、Character登場前からこれらを固定定数として
持っていた: `maxBackAttenDb=4dB`、`backCutoffHz=5000Hz`、
`maxBackDelayMs=8ms`。これは仕様書が提案する"Natural"のいずれの数値とも
一致しない(特にcutoffは5000Hzで、提案レンジ12-16kHzより大幅に低い =
より暗い)。

仕様書の数値をそのまま"Natural"として採用すると、Character登場前の
既存プロジェクトが、Character=Natural(新パラメータの既定値)で読み込んだ
際に**音が変わってしまう**。これはStereo Preserve(ADR-004)・Bass Anchor
(ADR-005)で確立した「新パラメータは、それが存在しなかった過去の
プロジェクトの音を変えない」という原則に反する。

## 決定

仕様書の絶対数値ではなく、**既存の固定定数をそのまま"Natural"として
採用する**。Vivid/Deepは、そこから同じ方向(より強い減衰・より暗い
カットオフ・より長いディレイ)へ2段階進める、新たに決定した数値とする:

| | 減衰(dB) | back cutoff(Hz) | back delay(ms) |
|---|---:|---:|---:|
| Natural(既定・従来と同一) | 4.0 | 5000 | 8 |
| Vivid | 5.0 | 4000 | 10 |
| Deep | 6.5 | 3000 | 14 |

これにより、Character=Natural(新規インスタンスの既定値でもある)は
既存の挙動とビット単位で同一になり、**schemaバージョンの更新も
移行ロジックも不要**になった(`Tests/CharacterTests.cpp`の
"Character = Natural is bit-identical to never having Character at all"
で検証)。

Vivid/Deepの数値は仕様書の絶対値と一致しないが、方向性(より強く
色づけされる)は仕様書の意図と一致している。実際の音質は試聴で
検証されておらず、`MANUAL_REQUIRED.md`に記載した。

## スコープを絞った実装(§6.2の部分実装)

仕様書§6.2は「単純な一段LPFのみでは音色変化が強く不自然になりやすい」
として、高域シェルフ・緩いLPF・中高域プレゼンスの3段構成を提案している。
今回は既存の一段LPFのカットオフをCharacterごとに変える(上表)のみを
実装し、シェルフ/プレゼンスEQの追加は**実装していない**。理由:
- 既存の一段LPF構造を変えずに済み、影響範囲が狭い。
- シェルフ/プレゼンスEQは新たなフィルタ設計・状態・テストが必要で、
  今回のセッションで実装した他の項目(Stereo Preserve再検証、
  Bass Anchor、Mix Law)と比べて優先度を判断する材料(実測・試聴)が
  今はない。
- カットオフ変更だけでも`Tests/CharacterTests.cpp`で実測可能な、
  方向性が正しい可聴差を作れている。

## 理由

- 音質: Naturalが既存プロジェクトの音を一切変えない。Vivid/Deepは
  明確に方向性の異なる新しい選択肢を追加する。
- 互換性: schemaバージョン更新不要、移行ロジック不要。既存の
  `Tests/BaselineRegressionTests.cpp`は無改造で通過する
  (Characterのデフォルトが既存定数と同一のため)。
- 実装難度・テスト可能性: 既存の`maxBackAttenDb`/`backCutoffHz`/
  `maxBackDelayMs`を`static constexpr`からブロック単位で更新される
  通常メンバへ変更するだけで済み、`process()`本体への変更は最小限。

## 影響

### 良い影響

- Character追加は既存プロジェクトに一切影響しない。
- Vivid/Deepは背後位置での音色変化を明確に強められる新しい表現力。

### 悪い影響

- 仕様書のNatural数値(12-16kHz)とは異なる値を採用したため、仕様書を
  そのまま参照する人には差異が分かりにくい可能性がある(本ADRで
  明記することで対応)。

### 互換性

- 新規パラメータID `character` 追加。既存パラメータIDは変更なし。
- schemaバージョンの更新なし(Natural=既定=従来動作のため不要)。

## 実装

- file: `Source/dsp/HelixEngine.{h,cpp}`(`maxBackAttenDb`/`backCutoffHz`/
  `maxBackDelayMs`をブロック単位で切り替え)、`Source/Parameters.h`
  (`characterID`, AudioParameterChoice)、`Source/PluginProcessor.cpp`、
  `Source/PluginEditor.{h,cpp}`(詳細タブへコンボボックス追加)、
  `Source/Presets.h`(全プリセットにNaturalを設定)

## テスト

- unit: `Tests/CharacterTests.cpp`(Natural=既存動作とビット単位一致、
  Vivid/Deepが背後位置で段階的に暗くなることの実測、サンプルレート・
  Twist最大値との組み合わせでの有限性、Mix 0%==Dryの維持)
- 全91件のCTest + ASan/UBSanがクリーン(0エラー)

## 再検討条件

実DAWでの試聴でVivid/Deepの音質・強さが不適切と判明した場合、または
§6.2のシェルフ/プレゼンスEQが必要と判断された場合に見直す。
