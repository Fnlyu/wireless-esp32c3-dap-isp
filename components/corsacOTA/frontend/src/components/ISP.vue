<template>
    <div class="
        flex flex-col
        pb-0
        
        
        rounded-lg
        shadow-md
        tracking-wide
        border border-blue
        ease-linear
        transition-all
        duration-150
        bg-gray-50
        
        ">
        <div class="p-5">
            <h2 class="text-xl font-bold text-gray-700 mb-4">ESP32C3 ISP 固件升级工具</h2>
            <!-- <h2 class="mt-5 pr-5 text-gray-500">ESP32C3 ISP 固件升级工具</h2> -->
            
            <!-- 连接设置 -->
            <div name="connection-settings" class="mb-5">
                <h3 class="text-md font-semibold text-gray-600 mb-2">连接设置</h3>
                <div class="flex items-center space-x-3 justify-center">
                    <div class="relative mt-5">
                        <label for="device-ip" class="mt-5 pr-5 text-gray-500">IP</label>
                        <input
                            type="text"
                            spellcheck="false"
                            name="ota address"
                            v-model="serverInfo.host"
                            id="device-ip"
                            class="
                                font-mono
                                px-3
                                py-3
                                placeholder-gray-400
                                text-gray-600
                                relative
                                bg-white
                                rounded
                                text-sm
                                border
                                outline-none
                                focus:outline-none focus:ring
                            "
                            placeholder="192.168.1.10"
                            />
                        <!-- <label for="device-ip" class="text-gray-600 mr-2">设备IP:</label> -->
                        <!-- <input type="text" id="device-ip" v-model="serverInfo.host"
                            class="border border-gray-300 rounded px-2 py-1 w-32" placeholder="192.168.1.10" /> -->
                    </div>
                    <div class="relative mt-5">
                        <label for="device-port" class="mt-5 pr-5 text-gray-500">Port</label>
                        <input type="text" id="device-port" v-model="serverInfo.port"
                            class="
                                font-mono
                                px-3
                                py-3
                                placeholder-gray-400
                                text-gray-600
                                relative
                                bg-white
                                rounded
                                text-sm
                                border
                                outline-none
                                focus:outline-none focus:ring
                            "
                            placeholder="3241" />
                        <!-- <label for="device-port" class="text-gray-600 mr-2">端口:</label> -->
                        <!-- <input type="text" id="device-port" v-model="serverInfo.port"
                            class="border border-gray-300 rounded px-2 py-1 w-16" placeholder="3241" /> -->
                    </div>
                    <button class="
                            text-blue-500
                            bg-transparent
                            border border-solid border-blue-500
                            hover:bg-blue-500 hover:text-white
                            disabled:text-gray-500 disabled:bg-gray-50
                            active:bg-blue-600
                            font-bold
                            uppercase
                            text-sm
                            mt-5
                            px-5
                            py-3
                            rounded
                            outline-none
                            focus:outline-none
                            mr-1
                            ml-5
                            ease-linear
                            transition-all
                            duration-150
                            "
                            :disabled="connecting"
                            @click="connectToDevice">
                        {{ connecting ? 'CONN...' : 'START !' }}
                    </button>
                </div>
                <div v-if="errorMsg" class="text-red-500 text-sm mt-1">{{ errorMsg }}</div>
            </div>

            <!-- ISP 设置 -->
            <div name="isp-settings" class="mb-5" v-if="connected || connecting">
                <h3 class="text-md font-semibold text-gray-600 mb-2">ISP 设置</h3>
                <div class="flex flex-wrap items-center space-x-4 mb-2">
                    <div class="flex items-center">
                        <label for="device-type" class="text-gray-600 mr-2">目标设备:</label>
                        <select id="device-type" v-model="ispOptions.deviceType"
                            class="border border-gray-300 rounded px-2 py-1" :disabled="uploading">
                            <option value="1">STM32</option>
                            <option value="2">ESP8266</option>
                            <option value="3">ESP32</option>
                            <!-- 可添加更多设备类型 -->
                        </select>
                    </div>
                    <div class="flex items-center">
                        <label for="baudrate" class="text-gray-600 mr-2">波特率:</label>
                        <select id="baudrate" v-model="ispOptions.baudrate"
                            class="border border-gray-300 rounded px-2 py-1" :disabled="uploading">
                            <option value="9600">9600</option>
                            <option value="19200">19200</option>
                            <option value="38400">38400</option>
                            <option value="57600">57600</option>
                            <option value="115200">115200</option>
                        </select>
                    </div>
                </div>
            </div>

            <!-- 固件文件选择 -->
            <div name="firmware-upload" class="mb-5" v-if="connected">
                <h3 class="text-md font-semibold text-gray-600 mb-2">固件选择</h3>
                <div class="flex items-center space-x-3">
                    <input type="file" id="firmware-file" ref="fileInput" class="hidden" @change="handleFileSelect" />
                    <button @click="triggerFileSelect" class="
                border border-gray-300
                bg-white
                hover:bg-gray-100
                text-gray-700
                px-4
                py-2
                rounded
                text-sm
              " :disabled="uploading">
                        选择固件文件
                    </button>
                    <span v-if="fileName" class="text-gray-600 text-sm">{{ fileName }}</span>
                    <span v-if="fileSize" class="text-gray-500 text-xs">({{ formatSize(fileSize) }})</span>
                </div>
            </div>

            <!-- 升级操作 -->
            <div name="upgrade-actions" class="mb-5" v-if="connected && fileName">
                <button @click="startUpgrade" v-if="!uploading" class="
              bg-green-500
              hover:bg-green-600
              text-white
              px-6
              py-2
              rounded
              text-sm
              mr-3
            " :disabled="!fileSelected">
                    开始升级
                </button>
                <button @click="cancelUpgrade" v-if="uploading"
                    class="bg-red-500 hover:bg-red-600 text-white px-6 py-2 rounded text-sm mr-3">
                    取消升级
                </button>
                <button @click="disconnect" v-if="!uploading" class="
              border border-gray-300
              bg-white
              hover:bg-gray-100
              text-gray-700
              px-4
              py-2
              rounded
              text-sm
            ">
                    断开连接
                </button>
            </div>

            <!-- 升级进度 -->
            <div name="upgrade-progress" v-if="uploading" class="mb-5">
                <div class="flex items-center justify-between mb-1">
                    <span class="text-sm text-gray-600">升级进度</span>
                    <span class="text-sm text-gray-600">{{ progressValue }}%</span>
                </div>
                <div class="w-full bg-gray-200 rounded-full h-2">
                    <div class="bg-blue-500 h-2 rounded-full transition-all duration-300"
                        :style="{ width: progressValue + '%' }"></div>
                </div>
                <div class="flex justify-between mt-1">
                    <span class="text-xs text-gray-500">
                        已发送: {{ formatSize(currentSize) }} / {{ formatSize(totalSize) }}
                    </span>
                    <span class="text-xs text-gray-500">速度: {{ formatSpeed }}</span>
                </div>
            </div>

            <!-- 日志输出 -->
            <div name="log-output" v-if="connected || errorMsg" class="mt-3">
                <h3 class="text-md font-semibold text-gray-600 mb-2">操作日志</h3>
                <div class="
              h-40
              overflow-y-auto
              bg-gray-100
              border border-gray-300
              rounded
              p-2
              text-sm
              font-mono
            " ref="logContainer">
                    <div v-for="(log, index) in logs" :key="index" :class="{
                'text-red-600': log.type === 'error',
                'text-yellow-600': log.type === 'warning',
                'text-green-600': log.type === 'success',
                'text-gray-800': log.type === 'info',
              }">
                        {{ log.time }} | {{ log.message }}
                    </div>
                </div>
            </div>

            <!-- 升级完成提示 -->
            <div v-if="upgradeComplete" class="
            mt-4
            p-3
            bg-green-100
            border border-green-300
            text-green-700
            rounded
            flex
            items-center
          ">
                <svg class="w-5 h-5 mr-2" fill="none" stroke="currentColor" viewBox="0 0 24 24"
                    xmlns="http://www.w3.org/2000/svg">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M5 13l4 4L19 7"></path>
                </svg>
                <span>固件升级成功完成！外部设备将重启并应用新固件。</span>
            </div>
        </div>
    </div>
</template>
  
  <script>
  import { ref, reactive, computed, onMounted, onUnmounted, nextTick } from 'vue'
  
  export default {
    name: 'IspUpgradeTool',
    setup() {
      // 连接状态
      const connecting = ref(false)
      const connected = ref(false)
      const errorMsg = ref('')
      
      // WebSocket
      let ws = null
      
      // 服务器信息
      const serverInfo = reactive({
        host: 'dap.local',
        port: '3241'
      })
      
      // ISP 选项
      const ispOptions = reactive({
        deviceType: '1', // 默认为STM32
        baudrate: '115200'
      })
      
      // 文件选择
      const fileInput = ref(null)
      const fileName = ref('')
      const fileSize = ref(0)
      const fileSelected = ref(false)
      let selectedFile = null
      
      // 升级状态
      const uploading = ref(false)
      const upgradeComplete = ref(false)
      const progressValue = ref(0)
      const totalSize = ref(0)
      const currentSize = ref(0)
      const uploadStartTime = ref(0)
      
      // 日志
      const logs = ref([])
      const logContainer = ref(null)
      
      // 默认分块大小
      const chunkSize = ref(1024 * 12)
      
      // 格式化文件大小
      const formatSize = (bytes) => {
        if (bytes < 1024) return bytes + ' B'
        else if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(2) + ' KB'
        else return (bytes / (1024 * 1024)).toFixed(2) + ' MB'
      }
      
      // 计算上传速度
      const formatSpeed = computed(() => {
        if (!uploading.value || !uploadStartTime.value) return '0 KB/s'
        
        const elapsedSeconds = (Date.now() - uploadStartTime.value) / 1000
        if (elapsedSeconds <= 0) return '0 KB/s'
        
        const bytesPerSecond = currentSize.value / elapsedSeconds
        return formatSize(bytesPerSecond) + '/s'
      })
      
      // 添加日志
      const addLog = (message, type = 'info') => {
        const time = new Date().toLocaleTimeString()
        logs.value.push({ time, message, type })
        
        // 滚动到最新日志
        nextTick(() => {
          if (logContainer.value) {
            logContainer.value.scrollTop = logContainer.value.scrollHeight
          }
        })
      }
      
      // 触发文件选择
      const triggerFileSelect = () => {
        fileInput.value.click()
      }
      
      // 处理文件选择
      const handleFileSelect = (event) => {
        const files = event.target.files
        if (files.length > 0) {
          selectedFile = files[0]
          fileName.value = selectedFile.name
          fileSize.value = selectedFile.size
          fileSelected.value = true
          totalSize.value = selectedFile.size
          
          addLog(`已选择固件文件: ${fileName.value} (${formatSize(fileSize.value)})`)
        }
      }
      
      // 连接到设备
      const connectToDevice = () => {
        if (connecting.value) return
        
        // 验证输入
        if (!serverInfo.host) {
          errorMsg.value = '请输入设备IP地址'
          return
        }
        
        if (!serverInfo.port || isNaN(serverInfo.port)) {
          errorMsg.value = '请输入有效的端口号'
          return
        }
        
        connecting.value = true
        errorMsg.value = ''
        logs.value = []
        addLog(`正在连接到 ${serverInfo.host}:${serverInfo.port}...`)
        
        // 创建 WebSocket 连接
        const wsUrl = `ws://${serverInfo.host}:${serverInfo.port}`
        
        try {
          ws = new WebSocket(wsUrl)
          
          ws.onopen = () => {
            connecting.value = false
            connected.value = true
            addLog('连接成功', 'success')
          }
          
          ws.onmessage = (event) => {
            handleMessage(event.data)
          }
          
          ws.onclose = () => {
            if (connecting.value) {
              addLog('连接失败', 'error')
              errorMsg.value = '无法连接到设备，请检查IP和端口是否正确'
            } else if (uploading.value && !upgradeComplete.value) {
              addLog('连接已关闭', 'warning')
              errorMsg.value = '连接已关闭，升级可能未完成'
            } else if (!upgradeComplete.value) {
              addLog('连接已关闭')
            }
            
            connecting.value = false
            connected.value = false
            uploading.value = false
            ws = null
          }
          
          ws.onerror = (error) => {
            addLog(`WebSocket 错误: ${error}`, 'error')
            errorMsg.value = '连接错误，请重试'
          }
        } catch (error) {
          connecting.value = false
          addLog(`连接错误: ${error.message}`, 'error')
          errorMsg.value = '无法创建连接'
        }
      }
      
      // 处理WebSocket消息
      const handleMessage = (data) => {
        try {
          // 解析消息
          const message = parseMessage(data)
          
          if (!message) {
            addLog('无法解析服务器消息', 'warning')
            return
          }
          
          if (message.code !== 0) {
            addLog(`错误: ${message.data?.msg || '未知错误'}`, 'error')
            return
          }
          
          // 更新进度
          if (message.data?.offset) {
            const offset = parseInt(message.data.offset)
            currentSize.value = offset
            progressValue.value = Math.floor((offset / totalSize.value) * 100)
          }
          
          // 处理状态
          if (message.data?.state) {
            switch (message.data.state) {
              case 'ready':
                sendNextChunk()
                break
              case 'done':
                upgradeComplete.value = true
                uploading.value = false
                progressValue.value = 100
                addLog('固件升级成功完成！', 'success')
                break
              case 'cancel':
                addLog('升级已取消', 'warning')
                uploading.value = false
                break
            }
          }
        } catch (error) {
          addLog(`处理消息错误: ${error.message}`, 'error')
        }
      }
      
      // 解析服务器消息
      const parseMessage = (str) => {
        // 基本消息格式检查
        const regex = /^code=-?\d*&data=\".*\"/gm
        if (regex.exec(str) == null) {
          return null
        }
        
        // 提取代码
        let codeEnd = str.indexOf('&')
        let code = parseInt(str.substring(5, codeEnd))
        
        // 提取数据
        let dataStart = str.indexOf('"') + 1
        let dataEnd = str.lastIndexOf('"')
        let dataStr = str.substring(dataStart, dataEnd)
        
        // 解析数据
        let data = {}
        dataStr.split('&').forEach(pair => {
          let [key, value] = pair.split('=')
          if (key && value !== undefined) {
            data[key] = value
          }
        })
        
        return { code, data }
      }
      
      // 开始升级
      const startUpgrade = async () => {
        if (!connected.value || !selectedFile || uploading.value) return
        
        // 检查文件有效性
        if (!await validateFirmware(selectedFile)) {
          addLog('无效的固件文件，请选择正确的固件', 'error')
          errorMsg.value = '固件文件无效'
          return
        }
        
        uploading.value = true
        upgradeComplete.value = false
        currentSize.value = 0
        progressValue.value = 0
        uploadStartTime.value = Date.now()
        
        // 发送升级开始命令
        let startCmd = `op=start&data=${fileSize.value}&target=isp&options=type=${ispOptions.deviceType}&baudrate=${ispOptions.baudrate}`
        
        addLog(`开始升级，目标设备: ${getDeviceTypeName()}, 波特率: ${ispOptions.baudrate}`)
        addLog(`固件大小: ${formatSize(fileSize.value)}`)
        
        if (ws && ws.readyState === WebSocket.OPEN) {
          ws.send(startCmd)
        } else {
          addLog('WebSocket 未连接', 'error')
          uploading.value = false
        }
      }
      
      // 获取设备类型名称
      const getDeviceTypeName = () => {
        switch (ispOptions.deviceType) {
          case '1': return 'STM32'
          case '2': return 'ESP8266'
          case '3': return 'ESP32'
          default: return `类型${ispOptions.deviceType}`
        }
      }
      
      // 验证固件文件
      const validateFirmware = async (file) => {
        // 这里可以添加更复杂的验证逻辑
        // 现在只是简单检查文件大小
        if (file.size <= 0) {
          return false
        }
        
        // 对于ESP固件，检查魔术字节
        try {
          const buffer = await readFileChunk(file, 0, 1)
          const view = new Uint8Array(buffer)
          
          // ESP32/ESP8266固件的魔术字节通常是0xE9
          if (ispOptions.deviceType === '2' || ispOptions.deviceType === '3') {
            return view[0] === 0xE9
          }
          
          // 对于其他设备类型，暂时不做特殊验证
          return true
        } catch (error) {
          addLog(`固件验证错误: ${error.message}`, 'error')
          return false
        }
      }
      
      // 读取文件块
      const readFileChunk = (file, start, length) => {
        return new Promise((resolve, reject) => {
          const reader = new FileReader()
          reader.onload = () => resolve(reader.result)
          reader.onerror = () => reject(reader.error)
          
          const blob = file.slice(start, start + length)
          reader.readAsArrayBuffer(blob)
        })
      }
      
      // 发送下一个数据块
      const sendNextChunk = async () => {
        if (!uploading.value || !selectedFile || !ws) return
        
        try {
          if (currentSize.value < fileSize.value) {
            const end = Math.min(currentSize.value + chunkSize.value, fileSize.value)
            const chunk = await readFileChunk(selectedFile, currentSize.value, end - currentSize.value)
            
            if (ws.readyState === WebSocket.OPEN) {
              ws.send(chunk)
            } else {
              throw new Error('WebSocket连接已关闭')
            }
          }
        } catch (error) {
          addLog(`发送数据错误: ${error.message}`, 'error')
          uploading.value = false
        }
      }
      
      // 取消升级
      const cancelUpgrade = () => {
        if (!uploading.value || !ws) return
        
        addLog('正在取消升级...')
        
        if (ws.readyState === WebSocket.OPEN) {
          ws.send('op=stop&data=')
        }
        
        uploading.value = false
      }
      
      // 断开连接
      const disconnect = () => {
        if (ws) {
          ws.close()
          ws = null
        }
        
        connected.value = false
        addLog('已断开连接')
      }
      
      // 组件卸载时清理资源
      onUnmounted(() => {
        if (ws) {
          ws.close()
          ws = null
        }
      })
      
      return {
        // 状态
        connecting,
        connected,
        errorMsg,
        serverInfo,
        ispOptions,
        fileInput,
        fileName,
        fileSize,
        fileSelected,
        uploading,
        upgradeComplete,
        progressValue,
        totalSize,
        currentSize,
        logs,
        logContainer,
        
        // 计算属性
        formatSpeed,
        
        // 方法
        formatSize,
        triggerFileSelect,
        handleFileSelect,
        connectToDevice,
        startUpgrade,
        cancelUpgrade,
        disconnect
      }
    }
  }
  </script>
  
  <style scoped>
  /* 可以添加自定义样式 */
  </style>