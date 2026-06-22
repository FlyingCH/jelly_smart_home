import { createRouter, createWebHistory } from 'vue-router'
import LoginView from '@/views/LoginView.vue'
import HomeView from '@/views/HomeView.vue'
import TemperatureView from '@/views/TemperatureView.vue'
import ElectricityView from '@/views/ElectricityView.vue'
import CarView from '@/views/CarView.vue'
import ConsoleView from '@/views/ConsoleView.vue'
import MapView from '@/views/MapView.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/login',
      name: 'login',
      component: LoginView,
      meta: { guest: true }
    },
    {
      path: '/',
      name: 'home',
      component: HomeView
    },
    {
      path: '/temperature',
      name: 'temperature',
      component: TemperatureView
    },
    {
      path: '/electricity',
      name: 'electricity',
      component: ElectricityView
    },
    {
      path: '/car',
      name: 'car',
      component: CarView
    },
    {
      path: '/console',
      name: 'console',
      component: ConsoleView
    },
    {
      path: '/map',
      name: 'map',
      component: MapView
    }
  ]
})

router.beforeEach((to, _from, next) => {
  const raw = localStorage.getItem('smart_home_auth')
  const isLoggedIn = raw ? JSON.parse(raw).loggedIn : false

  if (to.name === 'login') {
    if (isLoggedIn) {
      return next('/')
    }
    return next()
  }

  if (!isLoggedIn) {
    return next('/login')
  }

  next()
})

export default router
