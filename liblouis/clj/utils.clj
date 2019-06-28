(ns utils
  (:require [clojure.string :refer [join]])
  (:gen-class
     :name louis.utils
     :methods [^{:static true} [showDots ["[S" java.util.Map] String]]))

(defn -showDots [dots dot-map]
  (let [dot-map (into (sorted-map) dot-map)]
    (join "-" (map (fn [dot]
                     (apply str (or (seq (for [[k v] dot-map :when (not (= (bit-and k dot) 0))] v)) '(\0))))
                   dots))))
