<template>
  <div>

    <template v-if="data.item.status.transfer">

      <!-- If data is uploading, display progress bar -->
      <template v-if="!data.item.status.transfer.uploaded">
        <b-progress :value="data.item.status.transfer.upload_progress" max="100" show-progress animated variant="secondary" :id="'tooltip-upload_progress-'+data.item.id"></b-progress>

        <b-tooltip :target="'tooltip-upload_progress-'+data.item.id" triggers="hover">
          Upload progress
        </b-tooltip>
      </template>

      <!-- If data is storing, display progress bar -->
      <template v-if="!data.item.status.transfer.stored">
        <b-progress :value="data.item.status.transfer.store_progress" max="100" show-progress animated  variant="info" :id="'tooltip-store_progress-'+data.item.id"></b-progress>

        <b-tooltip :target="'tooltip-store_progress-'+data.item.id" triggers="hover">
          Store progress
        </b-tooltip>
      </template>

    </template>

    <!-- If process is running, display progress bar -->
    <template v-if="data.item.status.running">
      <b-progress :value="data.item.status.progress" max="100" show-progress animated variant="primary" :id="'tooltip-progress-'+data.item.id"></b-progress>

      <b-tooltip :target="'tooltip-progress-'+data.item.id" triggers="hover">
        Process progress
      </b-tooltip>
    </template>

    <!-- If process is not running, display status not started, success or fails with errors in tooltip -->
    <template v-else>

      <b-icon-check-circle v-if="data.item.status.done" variant="success" font-scale="2">></b-icon-check-circle>
      <b-icon-x-circle v-else-if="data.item.status.errors.length > 0" :id="'tooltip-'+data.item.id" variant="danger" font-scale="1.5">></b-icon-x-circle>

      <b-tooltip v-if="data.item.status.errors.length>0" :target="'tooltip-'+data.item.id" triggers="hover">
        <ul>
          <li v-for="error in data.item.status.errors" :key="error">
            {{error}}
          </li>
        </ul>
      </b-tooltip>

    </template>

  </div>

</template>

<script>

export default {
  name: 'ProcessStatus',
  props: {
    data: Object
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>

</style>

<style>

</style>
