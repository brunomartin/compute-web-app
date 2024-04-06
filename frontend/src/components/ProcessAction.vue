<template>
  <div>

    <template v-if="!status.running && !status.done">

      <template v-if="status.errors.length > 0">

        <b-button variant="outline-danger" style="border:none" @click="resetProcess()">
        <b-icon-arrow-clockwise font-scale="1.5"></b-icon-arrow-clockwise>
        </b-button>

      </template>

      <template v-else>

        <b-button variant="outline-info" style="border:none" @click="startProcess()">
        <b-icon-triangle-fill rotate="90" font-scale="1.5"></b-icon-triangle-fill>
        </b-button>

      </template>

    </template>

    <template v-else-if="status.running && !status.done">
      
      <b-button v-if="!status.suspended" variant="outline-secondary" style="border:none" @click="suspendProcess()">
        <b-icon-pause-fill font-scale="1.5"></b-icon-pause-fill>
      </b-button>
      
      <b-button v-if="status.suspended" variant="outline-secondary" style="border:none" @click="resumeProcess()">
      <b-icon-triangle-fill rotate="90" font-scale="1.5"></b-icon-triangle-fill>
      </b-button>
      
      <b-button v-if="status.suspended" variant="outline-danger" style="border:none" @click="stopProcess()">
      <b-icon-square-fill font-scale="1.5"></b-icon-square-fill>
      </b-button>

    </template>
    
    <template v-else>
      
      <b-button variant="outline-danger" style="border:none" @click="resetProcess()">
      <b-icon-arrow-clockwise font-scale="1.5"></b-icon-arrow-clockwise>
      </b-button>

    </template>
  </div>

</template>

<script>

import axios from 'axios';

export default {
  name: 'ProcessAction',
  props: {
    status: Object,
    processId: String
  },
  methods: {
    sendProcessAction(action_name) {
      let data = {
        version: '1.0',
        process_id: this.processId,
        name: action_name
      }

      let that = this

      axios
        .post(process.env.VUE_APP_CWA_API_URL + '/process_action', data)
        .then( () => {
          that.$emit('sent', this.processId)
        })
        .catch(error => {
          console.log('error on store');
                      throw error;
        });
    },
    startProcess() {
      this.sendProcessAction('start')
    },
    suspendProcess() {
      this.sendProcessAction('suspend')
    },
    resumeProcess() {
      this.sendProcessAction('resume')
    },
    stopProcess() {
    
      if(!confirm("Stop process " + this.processId + "?")) {
        return
      }

      this.sendProcessAction('stop')
    },
    resetProcess() {
    
      if(!confirm("Stop process " + this.processId + "?")) {
        return
      }

      this.sendProcessAction('reset')
    }
  }
  
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>

</style>

<style>

</style>
