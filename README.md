# RuntimeFileTexture

Windows のローカル画像・動画ファイルを選んで、メッシュのマテリアルテクスチャへ適用する Runtime プラグインです。

## できること

- ファイル選択ダイアログを表示
- 画像を `UTexture2D` として適用
- 動画を `UMediaPlayer` / `UMediaTexture` / `UFileMediaSource` で適用
- Level 上の `StaticMeshActor` を含む、MeshComponent を持つ Actor に対応

## Component の使い方

`URuntimeFileTextureComponent` を Actor に追加します。

設定項目:

- `TargetMesh`
  - 適用先の Actor を指定します
  - `StaticMeshActor` だけでなく、`UMeshComponent` を持つ Actor も使えます
- `TextureParameterName`
  - マテリアル内の Texture Parameter 名
  - 既定値は `Texture`
- `MaterialIndex`
  - 対象マテリアルスロット番号
- `bLoopVideo`
  - 動画をループ再生するかどうか

### Details からの操作

- `Select File` ボタンを押すと、ファイル選択ダイアログが開きます
- 選択したファイルが画像なら、そのまま Mesh のマテリアルへ適用します
- 動画なら MediaTexture として適用して再生します

### デバッグ表示

`Runtime File Texture | Debug` で以下を確認できます。

- `bLastOperationSucceeded`
- `LastFilePath`
- `LastType`
- `LastError`

## Blueprint Function Library

クラス名: `URuntimeFileTextureBPLibrary`

### `SelectFile`

```cpp
static bool SelectFile(FString& OutFilePath);
```

ファイル選択ダイアログを開いて、選択されたパスを返します。

使い方:

- まず `OutFilePath` を受け取る
- 成功したら、そのパスを `ApplyFileToMesh` に渡す

### `ApplyFileToMesh`

```cpp
static FRuntimeFileTextureResult ApplyFileToMesh(
    UMeshComponent* TargetMesh,
    FName TextureParameterName,
    const FString& FilePath,
    int32 MaterialIndex = 0,
    bool bLoopVideo = true
);
```

指定した Mesh に、画像または動画を適用します。

使い方:

1. 対象 Mesh を渡す
2. マテリアルの Texture Parameter 名を渡す
3. ファイルパスを渡す
4. 戻り値の `bSuccess` と `Error` を確認する

### 戻り値 `FRuntimeFileTextureResult`

- `bSuccess`
  - 成功なら `true`
- `FilePath`
  - 処理したファイルパス
- `Type`
  - `None` / `Image` / `Video`
- `Texture`
  - 適用したテクスチャ
- `Error`
  - 失敗時の理由

## 対応ファイル

画像:

- `.png`
- `.jpg`
- `.jpeg`
- `.bmp`
- `.tga`
- `.exr`

動画:

- `.mp4`
- `.mov`
- `.wmv`
- `.avi`
- `.m4v`

## 補足

- 動画再生の継続管理は Component 利用が推奨です
- 画像と動画は同じ Texture Parameter に差し替え可能です
