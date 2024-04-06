import Vue from 'vue'
import VueRouter from 'vue-router'
import Home from '../views/Home.vue'
import DataListView from '../views/DataListView.vue'
import ProcessListView from '../views/ProcessListView.vue'

Vue.use(VueRouter)

const routes = [
  {
    path: '/',
    name: 'Home',
    component: Home
  },
  {
    path: '/data-list',
    name: 'DataListView',
    component: DataListView
  },
  {
    path: '/process-list',
    name: 'ProcessListView',
    component: ProcessListView
  }
]

const router = new VueRouter({
  routes
})

export default router
