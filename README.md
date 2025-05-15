# 42cursus_Webserv🌐

この課題は、42Tokyo本科Common Core(基礎課程)のLevel5に位置するペア課題です。<br>
C++を用いてHTTPサーバを一から実装することで、HTTP通信の基本原理(HTTP1.1仕様)、ソケットプログラミング、ノンブロッキングI/O、CGI対応、多重化などの知識を実践的に学び、Nginxライクなサーバーを作成します。<br>
また、この課題の制作過程は別のプライベートリポジトリで作業しています。<br>
かつサーバーの挙動をテストするため、簡易的なフロントエンド、テストサイトも作成しています。

- [https://github.com/urabexon/Webserv-DevSite](https://github.com/urabexon/Webserv-DevSite)

## Usage💻
このプログラムの使用方法は以下の通りです。
1.  ** コンパイル方法 **
    - ターミナル上でリポジトリのルートディレクトリに移動し、make コマンドを実行してください。
    - 正しくコンパイルされると、実行ファイル（例: webserv）が生成されます。
2.  ** 実行方法 **
    - コンパイル後、生成された実行ファイルを実行します。
    - ポート8082に設定しているため、起動するとサーバーが立ち上がります。
3.  ** 動作確認方法 **
    - 上記リポジトリからWebserv-DevSiteをcloneし、make wを行ってください。
    - その後、webserv本体の実行ファイルを起動し、localhost:8082にアクセスしてください。

## Implementation Function🎓
このプロジェクトで実装した主な機能は以下の通りです。

- HTTPリクエスト解析
  - GET / POST / DELETE メソッドの対応、ヘッダーおよびボディの解析
- コンフィグファイルパーサー
  - 複数の仮想サーバー、locationブロック、エラーページ等を設定可能
- CGI対応(PHPなど)
  - Content-Type, Content-Length など環境変数を整備し、外部スクリプトを実行
- ノンブロッキングI/O & 多重接続処理
  - poll() を用いて複数クライアントへの同時応答を実現
- ファイルアップロード機能
  - multipart/form-data に対応し、任意のパスに保存可能
- ディレクトリリスティング（オートインデックス）
  - 該当ディレクトリに index.html がない場合に自動的に内容を表示
- リダイレクト処理
  - HTTPステータスコード 301 / 302 を返してURLを移動
- MIMEタイプ自動判定
  - 拡張子に応じた適切な Content-Type をレスポンスに付与

## Working Point💡
この課題では、以下の点を特に重視して開発を行いました。

- HTTP/1.1仕様に準拠した厳密なパーサー設計
- 様々なHTTPリクエストを想定した状態遷移型のリクエスト処理ロジック
- CGIのセキュアな呼び出しと、CGIプロセスのタイムアウト制御
- poll()を利用したI/O多重化におけるクライアントごとの状態管理
- コンフィグファイルの構文定義とネストされたlocationの解釈
- 自作のログ機能やエラーハンドリングの整備

## Points Learned📋
このプロジェクトを通して得た学びは以下の通りです。

- HTTPプロトコルとステートマシンによる解析手法の理解
- ソケット通信とpoll()による非同期処理の実装経験
- MIMEタイプ、レスポンスヘッダ、CGI環境変数の仕様理解
- リファクタリングと責務分離を意識したクラス設計
- 仕様にない異常系パターンの想定

## Directory Structure🌲
全体のディレクトリ構成です。

```
├── .gitignore
├── Makefile
├── etc
    └── webserv
    │   ├── 01_ports.conf
    │   ├── 02_hostnames.conf
    │   ├── 03_error_page.conf
    │   ├── 04_body_limit.conf
    │   ├── 05_routes.conf
    │   ├── 06_index.conf
    │   ├── 07_methods.conf
    │   ├── 08_multiple_ports.conf
    │   ├── 09_same_ports.conf
    │   ├── 10_multiple_servers.conf
    │   └── webserv.conf
├── inc
    ├── Cgi
    │   └── cgi_handler.h
    ├── Config
    │   ├── base_config.h
    │   ├── config_parser.h
    │   ├── http_config.h
    │   ├── location_config.h
    │   └── server_config.h
    ├── Exception
    │   └── http_exception.h
    ├── Request
    │   ├── file_upload.h
    │   ├── http_request.h
    │   ├── multipart_data.h
    │   └── request_parser.h
    ├── Response
    │   ├── http_response.h
    │   ├── mime_type.h
    │   └── response_builder.h
    ├── Util
    │   ├── SocketFd.h
    │   ├── config_utils.h
    │   ├── libft.h
    │   └── parsing_utils.h
    └── Web
    │   ├── client_connection.h
    │   ├── epoll_handler.h
    │   ├── event.h
    │   ├── http_server.h
    │   ├── server_manager.h
    │   └── server_socket.h
└── src
    ├── Cgi
        └── cgi_handler.cpp
    ├── Config
        ├── base_config.cpp
        ├── config_parser.cpp
        ├── http_config.cpp
        ├── location_config.cpp
        └── server_config.cpp
    ├── Exception
        └── http_exception.cpp
    ├── Request
        ├── file_upload.cpp
        ├── http_request.cpp
        ├── multipart_data.cpp
        └── request_parser.cpp
    ├── Response
        ├── http_response.cpp
        ├── mime_type.cpp
        └── response_builder.cpp
    ├── Util
        ├── SocketFd.cpp
        ├── config_utils.cpp
        ├── libft.cpp
        └── parsing_utils.cpp
    ├── Web
        ├── client_connection.cpp
        ├── epoll_handler.cpp
        ├── http_server.cpp
        ├── server_manager.cpp
        └── server_socket.cpp
    └── main.cpp
```


## Development Members🧑‍💻
- hurabe(コンフィグパーサー、HTTPリクエスト処理、CGI実装、検証サイト連携)
- jongykim(プロジェクト全体構成、仕様整理、keepalive実装、CGIスクリプト出力全体設計、検証サイト作成)

## Development Period📅
- 2025/02/19~2025/04/19 (59日間 / 約2ヶ月)

## Reference🔖
- [RFC 2616 - Hypertext Transfer Protocol -- HTTP/1.1](https://datatracker.ietf.org/doc/html/rfc2616)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [Nginx公式ドキュメント](https://nginx.org/en/docs/)
- [https://github.com/urabexon/Webserv-DevSite](https://github.com/urabexon/Webserv-DevSite)