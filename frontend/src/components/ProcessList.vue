<template>

  <div class="process-list">

    <loading :active.sync="isLoading" :can-cancel="false" :is-full-page="true"></loading>

    <b-form-checkbox v-model="periodicRetrieve">Watch Processes</b-form-checkbox>

    <b-table small striped show-empty bordered stacked="sm" :items="items" :fields="fields" :sort-by.sync="sortBy" :sort-desc.sync="sortDesc">

      <template v-slot:cell(status)="data" style="text-align: center">
        <ProcessStatus :data="data"/>
      </template>

      <template v-slot:cell(info)="data">
        <ResultInfo :processId="data.item.id" :process="data"/>
      </template>

      <template v-slot:cell(action)="data">

        <ProcessAction :processId="data.item.id" :status="data.item.status" @sent="onActionSent(data.item.id)"/>

      </template>

    </b-table>

  </div>

</template>

<script>
import axios from 'axios';

import Loading from 'vue-loading-overlay'
import 'vue-loading-overlay/dist/vue-loading.css'

import ProcessAction from '@/components/ProcessAction.vue';
import ProcessStatus from '@/components/ProcessStatus.vue';
import ResultInfo from '@/components/ResultInfo.vue';

export default {
  name: 'ProcessList',
  props: {
  },
  data: function() {
    return {
      items: [],
      fields: [
        {key: 'id', label:'Process Id', sortable: true},
        {key: 'post_time', label:'Post Time', sortable: true, formatter: "formatDateAssigned"},
        {key: 'data_id', label:'Data Id', sortable: true},
        {key: 'process_definition_id', label:'Process Definition', sortable: true},
        {key: 'action', label:'Action'},
        {key: 'status', label:'Status', sortable: true, class: ['text-center', 'align-middle']},
        {key: 'info', label:'Info'},
      ],
      sortBy: 'post_time',
      sortDesc: true,
      isLoading: false,
      runningProcessIds: [],
      interval: null,
      periodicRetrieve: true
    }
  },
  methods: {
    objectToList(object) {
      let list = []
      for(let index in object) {

        let parameter = {
          name: index,
          value: object[index]
        }
        list.push(parameter)
      }

      return list;
    },
    formatDateAssigned(value) {
      if(!value) {
        return ''
      }

      var options = { year: '2-digit', month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit', second: '2-digit' };
      return value.toLocaleDateString(undefined, options);
      // return value.toLocaleDateString('en-EN', options);
    },
    retrieveProcesses() {
      this.isLoading = true
      axios
        .get(process.env.VUE_APP_CWA_API_URL + '/process')
        .then( (response) => {
          let items = response.data

          // convert parameters object to list
          // schedule a retrieve if process is running
          for(let item_index in items) {
            let item = items[item_index]

            item.parameters = this.objectToList(item.parameters)

            // convert datetimes
            item.post_time = new Date(item.post_time)
            if(item.start_time) item.start_time = new Date(item.start_time)
            if(item.end_time) item.end_time = new Date(item.end_time)

            item.status.transfer = {
              uploaded: true,
              upload_progress: 100,
              stored: true,
              store_progress: 100
            }

            if(item.status.running) {
              this.retrieveProcess(item.id)
            }

          }

          this.items = items

          this.isLoading = false

        })
        .catch(error => {
          console.log('error retrieveProcesses()')
          throw error
        });
    },
    retrieveProcess(process_id) {
      axios
        .get(process.env.VUE_APP_CWA_API_URL + '/process/' + process_id)
        .then( (response) => {

          console.log(response)

          // find target item
          let item = this.items.find(x => x.id === process_id)

          // if not found, add it
          if(!item) {
            item = response.data
            this.items.push(item)
          }

          if(item.status.transfer) {
            response.data.status.transfer = item.status.transfer
          }

          // assign response content to found item
          item = Object.assign(item, response.data)

          // convert parameters to list for table display
          item.parameters = this.objectToList(item.parameters)

          // convert datetimes
          item.post_time = new Date(item.post_time)
          if(item.start_time) item.start_time = new Date(item.start_time)
          if(item.end_time) item.end_time = new Date(item.end_time)
          
          // schedule a retrieve if still running or remove it from
          // the running list
          if(item.status.running && !item.status.suspended) {

            // If data is not already stored, ask it
            if(!item.status.transfer) {
              this.retreiveDataTransferStatus(item)
            } else {
              if(!item.status.transfer.stored) {
                this.retreiveDataTransferStatus(item)
              }
            }

            if(!this.runningProcessIds.includes(process_id)) {
              console.log('new running process_id : ' + process_id)
              this.runningProcessIds.push(process_id)
            }
            
            setTimeout(() => this.retrieveProcess(process_id), 1000)
          } else {
            const index = this.runningProcessIds.indexOf(process_id);
            if (index > -1) {
              this.runningProcessIds.splice(process_id, 1);
            }

            item.status.transfer = {
              uploaded: true,
              upload_progress: 100,
              stored: true,
              store_progress: 100
            }
          }

        })
        .catch(error => {
          console.log('error retrieveProcess()')
          throw error
        });
    },
    retreiveDataTransferStatus(item) {

      axios
        .get(process.env.VUE_APP_CWA_API_URL + '/data/' + item.data_id)
        .then( (response) => {
          response
          item.status.transfer = response.data.status
        })
        .catch(error => {
          console.log('error retreiveDataTransferStatus()')
          throw error
        });
    },
    retrieveRunningProcesses() {
      axios
        .get(process.env.VUE_APP_CWA_API_URL + '/process_running')
        .then( (response) => {
          let process_ids = response.data

          for(let index in process_ids) {
            let process_id = process_ids[index]

            if(!this.runningProcessIds.includes(process_id)) {
              console.log('new running process_id : ' + process_id)
              this.runningProcessIds.push(process_id)
              this.retrieveProcess(process_id)
            }
          }

        })
        .catch(error => {
          console.log('error retrieveRunningProcesses()')
          throw error
        });
    },
    onActionSent(process_id) {
      console.log('retrieveProcess: ' + process_id)
      this.retrieveProcess(process_id)
    }
  },
  components: {
    Loading,
    ProcessAction,
    ProcessStatus,
    ResultInfo
  },
  mounted() {
    this.retrieveProcesses()

    // start periodic retrieve if default set to true
    if(this.periodicRetrieve && !this.interval) {
      this.interval = setInterval(() => this.retrieveRunningProcesses(), 1000)
    }

  },
  beforeDestroy () {
    clearInterval(this.interval)
  }, watch: {
    periodicRetrieve: function(value) {
      if(value) {
        if(!this.interval) {
          this.interval = setInterval(() => this.retrieveRunningProcesses(), 1000)
        }
      } else {
        clearInterval(this.interval)
        this.interval = null
      }
    }
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>

</style>

<style>

.hidden_header {
  display: none;
}

</style>
