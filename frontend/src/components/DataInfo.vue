<template>

  <div>

    <b-button v-b-modal="'info-modal-'+data.item.id" variant="outline-info" style="border:none">
      <b-icon-info-circle-fill font-scale="1.5"></b-icon-info-circle-fill>
    </b-button>

    <b-modal
      :id="'info-modal-'+data.item.id"
      :title="'Information on data '+data.item.id"
      ok-only
      size="lg"
      >

      <b-table-simple small fixed>
        <b-tbody>
          <b-tr>
            <b-td>Filename</b-td>
            <b-td>{{data.item.filename}}</b-td>
          </b-tr>
          <b-tr>
            <b-td>Name</b-td>
            <b-td>{{data.item.name}}</b-td>
          </b-tr>
          <b-tr>
            <b-td>Status</b-td>
            <b-td><DataStatus :data="currentData"/></b-td>
          </b-tr>
          <b-tr>
            <b-td>Download</b-td>
            <b-td>
              <b-link :href="downloadLink">
                <b-icon-download font-scale="2">></b-icon-download>
              </b-link>
            </b-td>
          </b-tr>
          <b-tr>
            <b-td>Upload end time</b-td>
            <b-td>{{formatDateAssigned(data.item.upload_end_time)}}</b-td>
          </b-tr>
          <b-tr>
            <b-td>Store end time</b-td>
            <b-td>{{formatDateAssigned(data.item.store_end_time)}}</b-td>
          </b-tr>
          <b-tr>
            <b-td>Uploaded bytes</b-td>
            <b-td>{{(data.item.uploaded_bytes/1024/1024).toFixed(2)}} MB</b-td>
          </b-tr>
          <b-tr>
            <b-td>Upload duration</b-td>
            <b-td>{{ getDuration(data.item.upload_end_time, data.item.post_time) }}s</b-td>
          </b-tr>
          <b-tr>
            <b-td>Upload rate</b-td>
            <b-td>{{ getRate(data.item.upload_end_time, data.item.post_time, data.item.uploaded_bytes) }} MB/s</b-td>
          </b-tr>
          <b-tr>
            <b-td>Stored bytes</b-td>
            <b-td>{{(data.item.stored_bytes/1024/1024).toFixed(2)}} MB</b-td>
          </b-tr>
          <b-tr>
            <b-td>Store duration</b-td>
            <b-td>{{ getDuration(data.item.store_end_time, data.item.post_time) }}s</b-td>
          </b-tr>
          <b-tr>
            <b-td>Store rate</b-td>
            <b-td>{{ getRate(data.item.store_end_time, data.item.post_time, data.item.stored_bytes) }} MB/s</b-td>
          </b-tr>
        </b-tbody>            
      </b-table-simple>

      <DatasetsViewer type="data" :data="data.item" :datasetNames="datasetNames"/>
      
    </b-modal>

  </div>

</template>

<script>

import DataStatus from '@/components/DataStatus.vue';
import DatasetsViewer from '@/components/DatasetsViewer.vue';

export default {
  name: 'DataInfo',
  components: {
    DatasetsViewer,
    DataStatus
  },
  props: {
    data: Object,
  },
  computed: {
    datasetNames: function() {
      let result = []
      this.data.item.datasets.forEach(element => {
        result.push(element.name)
      });
      return result
    },
    downloadLink: function() {
      let result = process.env.VUE_APP_CWA_API_URL
      result += '/data/' +  this.data.item.id + '/file'
      return result
    }
  },
  data: function() {
    return {
      currentData: Object
    }
  },
  methods: {
    formatDateAssigned(value) {
      
      // check if date is valid, return null string if not
      if(!(value instanceof Date && !isNaN(value))) {
        return ''
      }

      var options = { year: '2-digit', month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit', second: '2-digit' };
      return value.toLocaleDateString(undefined, options);
      // return value.toLocaleDateString('en-EN', options);
    },
    getDuration(date1, date2) {
      return Math.abs(date1 - date2)/1000
    },
    getRate(date1, date2, bytes) {
      let rate = (bytes/1024/1024) / (Math.abs(date1 - date2)/1000)
      return rate.toFixed(2)
    },
  },
  mounted() {
    this.currentData = this.data
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
</style>
