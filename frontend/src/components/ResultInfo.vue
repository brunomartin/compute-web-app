<template>

  <div>
  
    <b-button v-b-modal="'result-info-modal-'+process.item.id" variant="outline-info" style="border:none">
      <b-icon-info-circle-fill font-scale="1.5"></b-icon-info-circle-fill>
    </b-button>

    <b-modal
      :id="'result-info-modal-'+process.item.id"
      :title="'Information on result '+process.item.id"
      ok-only
      size="lg"
      >

      <b-table-simple small fixed>
        <b-tbody>
          <b-tr>
            <b-td>Process Definition</b-td>
            <b-td>{{process.item.process_definition_id}}</b-td>
          </b-tr>
          <b-tr>
            <b-td>Parameters</b-td>
            <b-td>                      
              <b-table-lite small :items="process.item.parameters" table-variant="info">
              </b-table-lite>
            </b-td>
          </b-tr>
          <b-tr>
            <b-td>Status</b-td>
            <b-td><ProcessStatus :data="process"/></b-td>
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
            <b-td>Post time</b-td>
            <b-td>{{formatDateAssigned(process.item.post_time)}}</b-td>
          </b-tr>
          <b-tr>
            <b-td>Start time</b-td>
            <b-td>{{formatDateAssigned(process.item.start_time)}}</b-td>
          </b-tr>
          <b-tr>
            <b-td>End time</b-td>
            <b-td>{{formatDateAssigned(process.item.end_time)}}</b-td>
          </b-tr>
          <b-tr>
            <b-td>Process duration</b-td>
            <b-td>{{ getDurationStr(process.item.start_time, process.item.end_time) }}</b-td>
          </b-tr>
        </b-tbody>
      </b-table-simple>

      <p v-if="result">
      <DatasetsViewer type="result" :data="result" :datasetNames="datasetNames"/>
      </p>

    </b-modal>

  </div>

</template>

<script>

import axios from 'axios';
import DatasetsViewer from '@/components/DatasetsViewer.vue';
import ProcessStatus from '@/components/ProcessStatus.vue';

export default {
  name: 'ResultInfo',
  components: {
    DatasetsViewer,
    ProcessStatus
  },
  props: {
    processId: String,
    process: Object
  },
  computed: {
    datasetNames: function() {
      let result = []
      
      if(!this.result) {
        return result
      }

      this.result.datasets.forEach(element => {
        result.push(element.name)
      });
      return result
    },
    downloadLink: function() {
      let result = process.env.VUE_APP_CWA_API_URL
      result += '/result/' +  this.process.item.id + '/file'
      return result
    }
  },
  data: function() {
    return {
      result: null
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
    getDurationStr(date1, date2) {
      if(!date1 || !date2) {
        return ""
      }

      let duration_s = (date2 - date1)/1000
      return duration_s + "s"
    },
    getRate(date1, date2, bytes) {
      let rate = (bytes/1024/1024) / (Math.abs(date1 - date2)/1000)
      return rate.toFixed(2)
    },
    retreiveResult() {
      // This method tries to retrieve result file info.
      // If it has not been created by the process, it will issue
      // a 404 error. If all dataset are created on file construction,
      // there is no need to retrieve info again. So do until no 404 error
      // and no more.

      if(this.result) {
        return
      }

      axios
        .get(process.env.VUE_APP_CWA_API_URL + '/result/' + this.processId)
        .then( (response) => {
          this.result = response.data
        })
        .catch(error => {
          if (error.response.status == 404) {
            // Result file is not created yet
          }
        });
    }
  },
  mounted() {
    // We are sure the result file is created if progress
    // is not null
    if(this.process.item.status.progress > 0) {
      this.retreiveResult()     
    }  
  }, watch: {
    'process.item.status.progress': function(value) {
      // When process progress increase, we can try
      // to retrieve result file info as the process
      // should have created it
      if(value > 0) {
        this.retreiveResult()
      }
    }
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
</style>
