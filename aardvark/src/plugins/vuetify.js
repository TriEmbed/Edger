import 'vuetify/dist/vuetify.min.css'
import 'material-design-icons-iconfont/src/material-design-icons.scss'
//import "@mdi/js"
import Vue from 'vue'
import Vuetify from 'vuetify'
// import zhHans from 'vuetify/lib/locale/zh-Hans'

Vue.use(Vuetify)

export default new Vuetify({
  icons: {
    iconfont: 'md',
  },
  // },
  // lang: {
  //   locales: { 'zh-Hans': zhHans },
  //   current: 'zh-Hans',
  // },
})
