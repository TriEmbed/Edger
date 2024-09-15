<template ref="here">
  <div>
    <v-toolbar flat height="30" />
    <h1>
      Edger 0.2.0
    </h1>


    <div>
      <h4 v-if="!production">
        development
      </h4>
      <h4 v-else>
        Production
      </h4>
    </div>
    <span v-show="! showRescanButton"> {{ message }}</span>

    <div v-if="showRescanButton && production">
      <VueIp :ip="ip" :on-change="change" :port="port" />
      <v-btn />
      <v-btn @click="Add">
        add
      </v-btn>
    </div>
    <v-btn 
      v-if="showRescanButton" 
      @click="reScan">
      scan for ant ESPs
    </v-btn>

    <DataTable
      :key="componentKey"
      ref="table"
      :default-options="{
        sortBy: ['lastModifyTime'],
        sortDesc: [true],
      }"
      :headers="headers"
      :load-data="loadData"
      item-key="id"
    >
      <template #[`item.number`]="{ index }">
        {{ index + 1 }}
      </template>

      <template #[`item.actions`]="{ item }">
        <v-tooltip top>
          <template #activator="{ on, attrs }">
            <v-icon class="mr-4" color="blue darken-3" v-bind="attrs" @click="handleConnect(item)" v-on="on">
              rowing
            </v-icon>
          </template>
          <span>connect</span>
        </v-tooltip>
        <v-tooltip top>
          <template #activator="{ on, attrs }">
            <v-icon class="mr-4" color="blue darken-3" v-bind="attrs" @click="handleEdit(item)" v-on="on">
              edit
            </v-icon>
          </template>
          <span>information</span>
        </v-tooltip>
        <v-tooltip v-if="production" id="delete">
          <template #activator="{ on, attrs }">
            <v-icon class="mr-4" color="blue darken-3" v-bind="attrs" @click="handleDelete(item)" v-on="on">
              delete
            </v-icon>
          </template>
          <span>delete</span>
        </v-tooltip>
      </template>
    </DataTable>

    <AntSchema
      ref="antSchema"
      @addSuccess="handleAddSuccess"
      @editSuccess="handleEditSuccess"
    />
  </div>
</template>

<script>


import {deleteDevice} from '@/api/device'
import toast from '@/utils/toast'
import axios from 'axios'
import AntSchema from "@/views/home/modules/AntSchema.vue";
import {getInfo, getMenus} from "@/api/esp";
import store from "@/store";
import {AntMutations} from "@/store/modules/ant";
import VueIp from "@/components/VueIp.vue";
import cookies from "vue-cookies";


import {ref} from 'vue';

const componentKey = ref(0);

export default {
  name: 'Home',
  components: {
    VueIp,
    AntSchema,
  }
  ,
  data: () => ({
    items: [{title: 'rescan', action: "scanMDNSAddress"}],

    query: {
      name: '',
    },
    msg: "",
    pass: 0,
    showRescan: true,
    production: false,
    index: -1,
    componentKey: 0,
    searchList: null,
    ip: '127.0.0.1', // or null
    port: '8888', // or null
  }),


  computed: {
    showRescanButton: function () {
      return this.showRescan
    },

    message: function () {
      return this.msg;
    },


    infoData () {
      return "hello";

    },
    headers () {
      return AntSchema.header
    },
  },

  mounted () {
    // if (process.env.NODE_ENV === "development") {
    //   let k = document.getElementById("idEnter")
    //   k.style.visibility = "hidden";
    //   k = document.getElementById("idValue")
    //   k.style.visibility = "hidden";
    // }
    // if (process.env.NODE_ENV === "development") {
    //   console.log("development")
    // }
    // if (process.env.NODE_ENV === "production") {
    //   console.log("production")
    // }
    this.production = process.env.NODE_ENV === "production"
    this.production = false
    if (!this.production) {
      console.log("development")

      this.searchList = ["esp-home.local",
        "esp-home-1.local",
        "esp-home-2.local",
        "esp-home-3.local",
        "esp-home-4.local",
        "esp-home-5.local",
        "esp-home-6.local",
        "esp-home-7.local",
        "esp-home-8.local",
        "esp-home-9.local",
        "esp-home-10.local",
        "esp-home-11.local",
        "esp-home-12.local",
        "esp-home-13.local",
        "esp-home-14.local",
        "esp-home-15.local",
        "esp-home-16.local",
        "esp-home-17.local",
        "esp-home-18.local",
        "esp-home-19.local",
      ];
    } else {

      console.log("production")
      const ip = cookies.get("ESP");
      this.searchList = JSON.parse(ip);
    }

    console.log('mounted called!')
  },

  beforeMount () {
    console.log('beforeMount called!')
  },

  destroyed () {
    console.log('destroyed called!')
  },

  beforeDestroy () {
    console.log('beforeDestroy called!')
  },


  async loadGetInfo () {
    await getInfo(null)
  },
  async loadGetMenu () {
    //  await getMenus(null)
  },
  methods: {

    /**
     * Will be called on changes to IP address or port
     */
    change (ip, port, valid) {
      this.ip = ip.toString()
      console.log(ip, port, valid);
    },

    Add () {
      const searchList = this.ip;
      const ip = cookies.get("ESP");
      console.log(ip)
      var array
      if (ip) {
        array = JSON.parse(ip)
        if (array.indexOf(searchList) != -1) {
          return;
        }
      } else {
        array = []
      }


      console.log(this.ip)
      array.push(this.ip)
      console.log(JSON.stringify(array))
      this.searchList = array;
      cookies.set("ESP", JSON.stringify(array), 100000)


    },
    scanMDNSAddress (index) {
      // Get all connected USB devices the website has been granted access to.
      //   navigator.usb.getDevices().then(devices => {
      //     devices.forEach(device => {
      //       console.log(device.productName);      // "Arduino Micro"
      //       console.log(device.manufacturerName); // "Arduino LLC"
      //     });
      //   }).


      /* here we look for any esp32 that may be out there
     */


      // loop being -1 means stop
      if ((index === -1) || (index >= this.searchList.length)) {
        index = -1
        this.showRescan = true;
        return
      }


      if (index === 0) {
        store.dispatch(`ants/${AntMutations.ANTS}`, null)

      }

      // and the number to the address
      // not necessary for the first one
      const address = this.searchList[index]
      if (this.pass === 3) {
        this.pass = 0
        index++;
      } else {
        this.pass++
      }

      this.msg = "mDNS Scanning for " + address + " pass " + this.pass

      axios.defaults.baseURL = "http://" + address;

      console.log("looking for", axios.defaults.baseURL)

      axios.get('/api/v1/system/info', {timeout: 200})
        .then(response => {
          this.pass = 0
          index++
          console.log("found", address)

          response.data["mdnsName"] = address
          store.dispatch(`ants/${AntMutations.ANTS}`, response.data).then(r => {
            console.log(axios.defaults.baseURL, response.data)
            this.componentKey++
          }).catch((error) => {
            console.log("mutation error", error)
          })
        }).catch(() => {
          console.log("not found", address)
        }).finally(() => {

          setTimeout(() => {
            this.scanMDNSAddress(index)
          }, 100)
        })
    },


    loadData: async function (options = {}) {
      return new Promise((resolve) => {
        try {
          const data = this.$store.state.ants.antList

          let dataLength = data.length

          if (dataLength === 0)
            return ({items: [], total: 0})
          dataLength = data.length
          console.log("scan new data", JSON.stringify(data[dataLength]));
          //     this.$refs['table'].refresh(true)
          resolve({items: data, total: dataLength})
        }catch (error)
        {
  console.log((error))
        }
      })
    },
    // action (index) {
    //   // this.items[index].action()
    //   //console.log(index)
    // },
    // //
    // /**
    //  * Added items
    //  * @return {Undefined}
    //  */
    // testGetMenu (id) {
    //   this.loadGetMenu().then(a => alert(a))
    // },
    //
    //
    // async loadGetInfo () {
    //   await getInfo(null)
    // },


    // async loadData (options = {}) {
    //   return getDeviceList({...this.query, ...options}).then(r => r.data)
    // },
    /**
     * Added items
     * @return {Undefined}
     * * here
     */
    reScan () {
      store.dispatch(`ants/${AntMutations.ANTS}`, null)
      this.index = 0;
      this.pass = 0
      this.showRescan = false
      this.scanMDNSAddress(this.index)
    },
    /**
     * Added device successfully
     * @return {Undefined}
     */
    handleAddSuccess () {
      toast.success({message: 'Add item successfully'})
      this.query = this.$options.data.apply(this).query
      this.$refs['table'].refresh(true)
    },
    /**
     * Edit item
     * @return {Undefined}
     * @param item
     */
    handleEdit (item) {

      console.log("AntSchema")
      //      let q = this.HomeItems.filter(d => d.tab === tab)[0].Schema

      this.$refs['antSchema'].open(item)
    },

    /**
     * connect item
     * @return {Undefined}
     * @param item
     */
    handleConnect (item) {

      // this call into device
      const addressMDNS = this.$store.state.ants.antList[item.id]["mdnsName"]
      console.log(addressMDNS)

      const routes = this.$router.getRoutes()

      const findDup = (index) => {
        if (name == null)
          return false
        return name.includes(addressMDNS)
      };
      const found = routes.find(route => findDup(route.name))

      if (found == null) {
        this.$store.state.ants.mDNSofCurrentAnt = addressMDNS;
        getMenus(addressMDNS)
      }
    },
    /**
     * Edit device success
     * @return {Undefined}
     */
    handleEditSuccess () {
      toast.success({message: 'Editing device successful'})
      this.$refs['table'].refresh()
    },
    /**
     * delete item
     * @param {Number | String} id item id
     * @return {Promise<Undefined>}
     */
    async handleDelete (id) {
      await deleteDevice(id)
      toast.success({message: 'Delete the item successfully'})
      await this.$refs.table.refresh()
    },


  },


  /**
   * Added items
   * @return {Undefined}
   */
  testGetMenu () {
    this.loadGetMenu().then(
      a => {
        alert(a)
      },
    )
  }
  ,


}
</script>

<style lang="scss">
</style>
