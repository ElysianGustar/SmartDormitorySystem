// OneNET 接入配置(集中管理,设备信息可入库)
// 敏感凭据(author_key / user_id)请填写到 config.local.js(已加入 .gitignore),不会提交
let local = {}
try {
    local = require('./config.local.js')
} catch (e) {
    local = {}
}

module.exports = {
    // 设备标识
    product_id: local.product_id || 'xUHsdh4wh3',
    device_name: local.device_name || 'test',
    // OneNET 用户凭据(敏感,请勿放进仓库)
    author_key: local.author_key || '',
    user_id: local.user_id || ''
}