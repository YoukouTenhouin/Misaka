# B 站账号与点播接口能力

本次使用匿名请求核验 Web 接口，只保留字段和状态。首版可先串起公开投稿的“选 P → 取流”，再用本人扫码会话验收私有数据。已有社区 SDK 仓库已宣布停止维护。[维护者声明](https://github.com/Nemo2011/bilibili-api/blob/3798d3b3bd3c3a93678d5a0367637a19262303ef/README.md)

| 操作 | 实测结果与首版落点 |
| --- | --- |
| 登录 | 生成接口返回二维码内容与轮询键。轮询端点测试遇到 HTTP 412。匿名导航接口返回业务码 -101 和 isLogin。首版以扫码后导航接口确认账号为登录成功条件；会话失效时重新扫码。具体有效期、确认与过期状态需本人账号验收。[生成二维码](https://passport.bilibili.com/x/passport-login/web/qrcode/generate) · [轮询](https://passport.bilibili.com/x/passport-login/web/qrcode/poll) · [导航](https://api.bilibili.com/x/web-interface/nav) |
| 历史与续播 | 游标接口匿名返回 -101。播放响应包含 last_play_time、last_play_cid。本人账号验收时需核实历史分页、记录对应的分 P、位置单位、已看完的表示。首版只读已有位置，之后按投稿与 cid 存本机进度。[历史](https://api.bilibili.com/x/web-interface/history/cursor) · [点播样本](https://api.bilibili.com/x/player/wbi/playurl?bvid=BV17x411w7KC&cid=279786&fnval=16&qn=80) |
| 收藏 | 通过用户标识查询创建的收藏夹，再按收藏夹标识分页读取资源。两个匿名探测均收到业务码 0，样本无数据。真实非空列表、私有收藏及失效投稿仍需用本人账号验证。[收藏夹](https://api.bilibili.com/x/v3/fav/folder/created/list-all?up_mid=2) · [夹内资源](https://api.bilibili.com/x/v3/fav/resource/list?media_id=1&pn=1&ps=20) |
| 投稿与分 P | 公开 BV 样本的分 P 接口返回 cid、page、part、duration，可用作稳定分 P 标识、顺序、名称及长度。投稿详情探测收到 HTTP 412，AV 输入和两种编号归一化仍需验收。[分 P 样本](https://api.bilibili.com/x/player/pagelist?bvid=BV17x411w7KC) · [投稿详情](https://api.bilibili.com/x/web-interface/view?bvid=BV17x411w7KC) |
| 清晰度与保存 | 匿名样本请求 qn=80，实际返回 quality=16。DASH 响应含视频、音频轨道，视频条目包含清晰度标识、编码和临时地址，地址带 deadline。选择器应以实际可用轨道为准；播放或保存时按分 P 解析新地址，只持久化投稿、cid、清晰度偏好和本地文件位置。账号的高画质需真实权益验证。[点播样本](https://api.bilibili.com/x/player/wbi/playurl?bvid=BV17x411w7KC&cid=279786&fnval=16&qn=80) |

建议分别呈现需重新登录、服务拒绝请求、账号可用清晰度较低和资源不可用。HTTP 412 时提示稍后重试。验收可选择一个含多 P 的收藏投稿，在网页观看第二 P 一分钟后，检查客户端能否从历史定位同一 P 和位置，再确认高画质播放及保存后的离线播放。
