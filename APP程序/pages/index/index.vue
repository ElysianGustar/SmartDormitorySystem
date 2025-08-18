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
		
			
        </view>
    </view>
</template>


<script>
    // 引入创建令牌的函数
    const { createCommonToken } = require('@/key.js')
	
	const my_product_id = "xUHsdh4wh3" //填自己产品号
	const my_device_name = "test"	//填自己设备号
	
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
                // 台灯状态
                led: true,
                // 认证令牌
                token: '',
                // 最后更新时间
                lastUpdateTime: 0,
            }
        },
        onLoad() {
            // 页面加载时生成认证令牌
            const params = {
				// 填自己用户秘钥
                author_key: '6Uk9bp+mybiZ2++0APrIbJnp7rdS8cSMj/klPJ1W26sWKBySRWJyrPRstVdJav6/',
				// 版本号不用改
                version: '2022-05-01',
				// 填自己用户id
                user_id: '426241',
            }
            this.token = createCommonToken(params);
        },
        onShow() {
            // 页面显示时，首次和定时获取设备数据
            this.fetchDevData();
            setInterval(()=>{
                this.fetchDevData();
            }, 3000) // 每3秒刷新一次数据
        },
        methods: {
            // 获取设备数据的方法
            fetchDevData() {
                uni.request({
                    url: 'https://iot-api.heclouds.com/thingmodel/query-device-property', // 示例接口地址
                    method: 'GET',
                    data: {
                        product_id: my_product_id,
                        device_name: my_device_name
                    },
                    header: {
                        'authorization': this.token // 使用认证令牌
                    },
                    success: (res) => {
                        // 更新温度、湿度和台灯状态数据
                        console.log(res.data);
                        this.temp = res.data.data[2].value;
                        this.humi = res.data.data[1].value;
                        this.MQ2 = res.data.data[0].value;
                        // 更新最后刷新时间
                        this.lastUpdateTime = Date.now();
                        // 判断设备是否在线（5分钟内有数据刷新则在线）
                        this.isOnline = (Date.now() - this.lastUpdateTime) <= 300000;
                    },
                    fail: (err) => {
                        // 请求失败时设置设备为离线状态
                        this.isOnline = false;
                        console.error('获取设备数据失败:', err);
                    }
                });
            },
            // 台灯状态改变时的处理方法
            onLedSwitch(event) {
                console.log(event.detail.value);
                let value = event.detail.value;
                uni.request({
                    url: 'https://iot-api.heclouds.com/thingmodel/set-device-property', // 示例接口地址
                    method: 'POST',
                    data: {
                        product_id: my_product_id,
                        device_name: my_device_name,
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
