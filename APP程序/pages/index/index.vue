<template>
    <view class="wrap">
        <!-- 设备区域容器，包含所有设备卡片 -->
        <view class="dev-area">
            <!-- 设备在线状态卡片 -->
            <view class="dev-cart status-card" :class="{'online': isOnline, 'offline': !isOnline}">
                <view class="">
                    <!-- 设备名称 -->
                    <view class="dev-name">设备状态</view>
                    <!-- 设备图标 -->
                    <view class="status-icon" :class="{'online-icon': isOnline, 'offline-icon': !isOnline}"></view>
                </view>
                <!-- 显示在线状态 -->
                <view class="dev-data">{{isOnline ? '在线' : '离线'}}</view>
            </view>

            <!-- 温度设备卡片，点击时跳转到折线图页面 -->
            <view class="dev-cart" @click="goLineChart">
                <view class="">
                    <!-- 设备名称 -->
                    <view class="dev-name">温度</view>
                    <!-- 设备图标 -->
                    <image class="dev-logo" src="../../static/carbon_temperature-hot.png" mode=""></image>
                </view>
                <!-- 显示温度数据 -->
                <view class="dev-data">{{temp}} ℃</view>
            </view>

            <!-- 湿度设备卡片，点击时跳转到折线图页面 -->
            <view class="dev-cart" @click="goLineChart">
                <view class="">
                    <!-- 设备名称 -->
                    <view class="dev-name">湿度</view>
                    <!-- 设备图标 -->
                    <image class="dev-logo" src="../../static/carbon_humidity-alt.png" mode=""></image>
                </view>
                <!-- 显示湿度数据 -->
                <view class="dev-data">{{humi}} %</view>
            </view>

		
			<!-- 二氧化碳设备卡片，点击时跳转到折线图页面 -->
			<view class="dev-cart" @click="goLineChart">
			    <view class="">
			        <!-- 设备名称 -->
			        <view class="dev-name">烟雾浓度</view>
			        <!-- 设备图标 -->
			        <image class="dev-logo" src="../../static/meteocons_extreme-smoke-fill.png" mode=""></image>
			    </view>
			    <!-- 显示湿度数据 -->
			    <view class="dev-data">{{MQ2}}ppm</view>
			</view>
		
			<!-- 远程控制卡片:开关下发 led 属性(设备端映射为蜂鸣器 PB10) -->
			<view class="dev-cart control-card">
			    <view class="">
			        <view class="dev-name">远程蜂鸣器</view>
			    </view>
			    <switch :checked="led" color="#1890FF" @change="onLedSwitch" />
			</view>
			
        </view>
    </view>
</template>


<script>
    // 引入创建令牌的函数
    const { createCommonToken } = require('@/key.js')
    // 统一配置(产品号 / 设备号 / 凭据)
    const config = require('@/config.js')

    export default {
        data() {
            return {
                // 设备在线状态
                isOnline: false,
                // 温度数据
                temp: '',
                // 湿度数据
                humi: '',
                // 二氧化碳
                MQ2: '',
                // 远程控制开关状态
                led: false,
                // 认证令牌
                token: '',
                // 最后更新时间
                lastUpdateTime: 0,
                // 定时器ID
                timer: null,
            }
        },
        onLoad() {
            // 页面加载时生成认证令牌
            const params = {
                author_key: config.author_key,
                version: '2022-05-01',
                user_id: config.user_id,
            }
            this.token = createCommonToken(params);
        },
        onShow() {
            // 页面显示时,首次和定时获取设备数据
            this.fetchDevData();
            if (!this.timer) {
                this.timer = setInterval(()=>{
                    this.fetchDevData();
                }, 3000) // 每3秒刷新一次数据
            }
        },
        onHide() {
            // 页面隐藏时清理定时器,避免与折线图页累计重复请求
            if (this.timer) {
                clearInterval(this.timer);
                this.timer = null;
            }
        },
        onUnload() {
            if (this.timer) {
                clearInterval(this.timer);
                this.timer = null;
            }
        },
        methods: {
            // 获取设备数据的方法
            fetchDevData() {
                uni.request({
                    url: 'https://iot-api.heclouds.com/thingmodel/query-device-property', // 示例接口地址
                    method: 'GET',
                    data: {
                        product_id: config.product_id,
                        device_name: config.device_name
                    },
                    header: {
                        'authorization': this.token // 使用认证令牌
                    },
                    success: (res) => {
                        // 按 identifier 匹配属性,避免依赖返回顺序
                        const body = res.data || {};
                        const props = Array.isArray(body.data) ? body.data
                                        : (Array.isArray(body.properties) ? body.properties : []);
                        const getProp = (id) => {
                            const item = props.find(p => p && p.identifier === id);
                            return item ? item.value : '';
                        };
                        console.log('props', props);
                        this.temp = getProp('temp');
                        this.humi = getProp('humi');
                        this.MQ2 = getProp('MQ2');

                        // 在线判断:取各属性上报时间戳中的最新值,超过5分钟视为离线
                        let lastTime = 0;
                        props.forEach(p => {
                            if (p && p.time !== undefined && p.time !== null && p.time !== '') {
                                const t = parseInt(p.time, 10);
                                if (t > lastTime) lastTime = t;
                            }
                        });
                        if (lastTime > 0) {
                            this.lastUpdateTime = lastTime;
                            this.isOnline = (Date.now() - lastTime) <= 300000;
                        } else {
                            // 接口未返回时间戳时,降级为请求成功且有数据即视为在线
                            this.lastUpdateTime = Date.now();
                            this.isOnline = props.length > 0;
                        }
                    },
                    fail: (err) => {
                        // 请求失败时设置设备为离线状态
                        this.isOnline = false;
                        console.error('获取设备数据失败:', err);
                    }
                });
            },
            // 远程开关状态改变时的处理方法(下发 led 属性)
            onLedSwitch(event) {
                console.log(event.detail.value);
                let value = event.detail.value;
                this.led = value;
                uni.request({
                    url: 'https://iot-api.heclouds.com/thingmodel/set-device-property', // 示例接口地址
                    method: 'POST',
                    data: {
                        product_id: config.product_id,
                        device_name: config.device_name,
                        params: {
                            "led": value
                        }
                    },
                    header: {
                        'authorization': this.token // 使用认证令牌
                    },
                    success: () => {
                        console.log('LED ' + (value ? 'ON' : 'OFF') + ' !');
                    }
                });
            },
            // 跳转到折线图页面的方法
            goLineChart() {
                // 将认证令牌存储在本地，供其他页面使用
                uni.setStorageSync('token', this.token);
                // 页面跳转
                uni.navigateTo({
                    url:`../LineChart/LineChart`
                });
            },
        }
    }
</script>


<style>
    .wrap {
        padding: 40rpx;
        background: linear-gradient(135deg, #f5f7fa 0%, #e4e8eb 100%);
        min-height: 100vh;
        display: flex;
        flex-direction: column;
        justify-content: flex-start;
        padding-top: 100rpx;
    }

    .dev-area {
        display: flex;
        flex-direction: column;
        gap: 30rpx;
        margin: 0 auto;
        width: 100%;
    }

    .dev-cart {
        height: 200rpx;
        width: 100%;
        border-radius: 20rpx;
        display: flex;
        justify-content: space-around;
        align-items: center;
        background: white;
        box-shadow: 0 8rpx 20rpx rgba(0, 0, 0, 0.1);
        transition: all 0.3s ease;
        position: relative;
        overflow: hidden;
    }

    .status-card {
        background: linear-gradient(135deg, #ffffff 0%, #f5f5f5 100%);
    }

    .control-card {
        justify-content: space-between;
        padding: 0 40rpx;
    }

    .status-card.online {
        border-left: 8rpx solid #67C23A;
    }

    .status-card.offline {
        border-left: 8rpx solid #F56C6C;
    }

    .status-icon {
        width: 90rpx;
        height: 90rpx;
        margin-top: 10rpx;
        border-radius: 50%;
        position: relative;
    }

    .online-icon {
        background: #67C23A;
        box-shadow: 0 0 20rpx rgba(103, 194, 58, 0.3);
    }

    .online-icon::after {
        content: '';
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        width: 30rpx;
        height: 30rpx;
        background: white;
        border-radius: 50%;
        animation: pulse 2s infinite;
    }

    .offline-icon {
        background: #F56C6C;
        box-shadow: 0 0 20rpx rgba(245, 108, 108, 0.3);
    }

    .offline-icon::after {
        content: '';
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        width: 30rpx;
        height: 30rpx;
        background: white;
        border-radius: 50%;
        animation: pulse 2s infinite;
    }

    @keyframes pulse {
        0% {
            transform: translate(-50%, -50%) scale(1);
            opacity: 1;
        }
        50% {
            transform: translate(-50%, -50%) scale(1.5);
            opacity: 0.5;
        }
        100% {
            transform: translate(-50%, -50%) scale(1);
            opacity: 1;
        }
    }

    .dev-cart::before {
        content: '';
        position: absolute;
        top: 0;
        left: 0;
        width: 100%;
        height: 100%;
        background: linear-gradient(45deg, rgba(255,255,255,0.1), rgba(255,255,255,0));
        pointer-events: none;
    }

    .dev-cart:active {
        transform: translateY(4rpx);
        box-shadow: 0 4rpx 10rpx rgba(0, 0, 0, 0.1);
    }

    .dev-name {
        font-size: 28rpx;
        text-align: center;
        color: #333;
        font-weight: 500;
        margin-bottom: 10rpx;
    }

    .dev-logo {
        width: 90rpx;
        height: 90rpx;
        margin-top: 10rpx;
        filter: drop-shadow(0 4rpx 6rpx rgba(0, 0, 0, 0.1));
    }

    .dev-data {
        font-size: 60rpx;
        font-weight: 600;
        color: #2c3e50;
        text-shadow: 0 2rpx 4rpx rgba(0, 0, 0, 0.1);
    }
</style>
