<template>
  <div>
    <template v-if="!data.item.status.stored">

      <b-progress :value="data.item.status.upload_progress" max="100" show-progress :animated="data.item.status.upload_progress<100" :id="'tooltip-upload_progress-'+data.item.id" variant="info"></b-progress>

      <b-progress :value="data.item.status.store_progress" max="100" show-progress animated :id="'tooltip-store_progress-'+data.item.id" variant="primary"></b-progress>

      <b-tooltip :target="'tooltip-upload_progress-'+data.item.id" triggers="hover">
        Upload progress
      </b-tooltip>

      <b-tooltip :target="'tooltip-store_progress-'+data.item.id" triggers="hover">
        Store progress
      </b-tooltip>

    </template>
    <template v-else>
      <b-icon-check-circle variant="success" font-scale="2">></b-icon-check-circle>
    </template>
  </div>
</template>

<script>

export default {
  name: 'DataStatus',
  props: {
    data: Object
  },
  data: function() {
    return {
      
    }
  },
  filters: {
    
  },
  methods: {
    
  },
  components: {
    
  },
  mounted() {
    
  },
  beforeDestroy () {
    
  }, watch: {
    
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
</style>
