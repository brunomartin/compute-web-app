<template>

<div class="process-view">

<ProcessList />

</div>

</template>

<script>
// @ is an alias to /src
import ProcessList from '@/components/ProcessList.vue';

export default {
  name: 'ProcessListView',
  props: {
  },
  data: function() {
    return {
    }
  },
  components: {
    ProcessList
  }
}
</script>

<style>

</style>