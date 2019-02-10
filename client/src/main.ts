import Vue from 'vue';
import App from './App.vue';
import router from './router';
import 'vue-resize/dist/vue-resize.css';
import VueResize from 'vue-resize';

Vue.config.productionTip = false;
Vue.use(VueResize);

new Vue({
  router,
  render: h => h(App),
}).$mount('#app');
