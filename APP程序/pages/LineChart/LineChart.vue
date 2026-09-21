<template>
	<view class="container">
		<!-- 图表显示区域 -->
		<view class="charts-box">
			<!-- qiun-data-charts 组件用于显示类型为line的折线图，传入的数据包括图表选项opts和图表数据chartData -->
			<qiun-data-charts type="line" :opts="opts" :chartData="chartData" />
		</view>
	</view>
</template>


<script>
	import config from '@/config.js'
	export default {
		data() {
			return {
				// 温度和湿度数据
				temp: '',
				humi: '',
				// 图表数据对象，包含时间轴(categories)和数据系列(series)
				chartData: {
					categories: [], // 时间轴，用于x轴显示
					series: [{ // 数据系列
							name: '温度',
							data: [] // 温度数据数组
						},
						{
							name: '湿度',
							data: [] // 湿度数据数组
						}
					]
				},
				// 图表选项
				opts: {
					color: ["#1890FF", "#91CB74"], // 系列颜色
					padding: [15, 10, 0, 15], // 图表内边距
					enableScroll: false, // 禁用滚动
					legend: {}, // 图例配置
					xAxis: {
						disableGrid: true, // 禁用网格线
						rotateLabel: true, // 旋转标签
					},
					yAxis: {
						gridType: "dash", // 网格线类型为虚线
						dashLength: 2 // 虚线段长
					},
					extra: {
						line: {
							type: "straight", // 线型为直线
							width: 2, // 线宽
							activeType: "hollow" // 激活点类型为空心
						}
					}
				},
				// 认证令牌
				token: '',
				// 定时器ID
				timer: null,
			}
		},
		onLoad(options) {
			// 页面加载时从本地存储获取认证令牌
			this.token = uni.getStorageSync('token');
		},
		onShow() {
			// 页面显示时获取设备数据，并设置2秒定时刷新数据
			this.fetchDevData();
			if (!this.timer) {
				this.timer = setInterval(() => {
					this.fetchDevData();
				}, 2000);
			}
		},
		onHide() {
			// 页面隐藏时清理定时器,避免重复请求
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

						let now = new Date();
						let hours = now.getHours().toString().padStart(2, '0');
						let minutes = now.getMinutes().toString().padStart(2, '0');
						let seconds = now.getSeconds().toString().padStart(2, '0');
						let currentTime = `${hours}:${minutes}:${seconds}`;

						// 更新温度和湿度数据
						this.temp = getProp('temp');
						this.humi = getProp('humi');
						// 限制数据点数量，避免数据过多
						if (this.chartData.categories.length >= 10) {
							this.chartData.categories.shift();
							this.chartData.series[0].data.shift();
							this.chartData.series[1].data.shift();
						}
						// 添加新的数据点
						this.chartData.categories.push(currentTime);
						this.chartData.series[0].data.push(this.temp);
						this.chartData.series[1].data.push(this.humi);
						// 触发图表更新
						this.chartData = {
							...this.chartData
						};
					}
				});
			},
		}
	}
</script>


<style>
	.container {
		display: flex;
		flex-direction: column;
	}

	.charts-box {
		width: 100%;
		height: 300px;
		/* 设置图表容器的高度 */
	}
</style>