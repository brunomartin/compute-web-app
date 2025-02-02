import Vue from 'vue'
import App from './App.vue'
import store from './store'
import router from './router'

import { BootstrapVue, IconsPlugin } from 'bootstrap-vue'

import bFormSlider from 'vue-bootstrap-slider';
import 'bootstrap-slider/dist/css/bootstrap-slider.css'

import axios from 'axios'
import VueAxios from 'vue-axios'

Vue.use(bFormSlider)

// Vue.use(axios)
Vue.use(VueAxios, axios)

Vue.config.productionTip = true

// Install BootstrapVue
Vue.use(BootstrapVue)
// Optionally install the BootstrapVue icon components plugin
Vue.use(IconsPlugin)

import 'bootstrap/dist/css/bootstrap.css'
import 'bootstrap-vue/dist/bootstrap-vue.css'

new Vue({
  store,
  router,
  render: h => h(App)
}).$mount('#app')
